#include "textmanager.h"
#include "ui_textmanager.h"

#include "lessonminer.h"
#include "lessonminercontroller.h"
#include "db.h"
#include "text.h"

#include "inc/sqlite3pp.h"

#include <iostream>

#include <QString>
#include <QFileDialog>
#include <QCryptographicHash>
#include <QByteArray>
#include <QSettings>
#include <QFile>
#include <QProgressBar>
#include <QModelIndex>
#include <QStandardItemModel>

TextManager::TextManager(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::TextManager),
        refreshed(false),
        sourcesModel(new QStandardItemModel),
        textsModel(new QStandardItemModel)
{
        ui->setupUi(this);

        QSettings s;

        ui->sourcesTable->setModel(sourcesModel);
        ui->textsTable->setModel(textsModel);

        // progress bar/text setup
        ui->progress->setRange(0,100);
        ui->progress->hide();
        ui->progressText->hide();

        ui->selectionMethod->setCurrentIndex(s.value("select_method").toInt());

        connect(ui->importButton, SIGNAL(clicked()),
                this,             SLOT  (addFiles()));
        // source tree signals
        connect(ui->refreshSources, SIGNAL(clicked()),
                this,               SLOT  (refreshSources()));

        connect(ui->textsTable,  SIGNAL(doubleClicked(const QModelIndex&)),
                this,            SLOT  (doubleClicked(const QModelIndex&)));
                        
        connect(ui->sourcesTable, SIGNAL(pressed(const QModelIndex&)),
                this,             SLOT  (populateTexts(const QModelIndex&)));

        connect(ui->selectionMethod, SIGNAL(currentIndexChanged(int)),
                this,                SLOT  (changeSelectMethod(int)));      
}

TextManager::~TextManager()
{
        delete ui;
        delete sourcesModel;
        delete textsModel;
}

void TextManager::changeSelectMethod(int i)
{
        QSettings s;
        if (s.value("select_method").toInt() != i)
                s.setValue("select_method", i);
}

void TextManager::tabActive(int i)
{
        if ((i == 1) && (!refreshed)) {
                // do stuff when this tab is made active & the tree hasnt been
                // loaded
                refreshSources();
        }
        if (i != 1) {
                //textsModel->clear();  
               // refreshed = false;   
        }
}

void TextManager::refreshSources()
{
        sourcesModel->clear();

        QStringList headers;
        headers << "id"
                << "Text"
                << "Length"
                << "Results"
                << "WPM"
                << "Dis";
        sourcesModel->setHorizontalHeaderLabels(headers);

        ui->sourcesTable->setSortingEnabled(false);
        ui->sourcesTable->setColumnHidden(0, true);

        QHeaderView* verticalHeader = ui->sourcesTable->verticalHeader();
        verticalHeader->sectionResizeMode(QHeaderView::Fixed);
        verticalHeader->setDefaultSectionSize(24);
        
        refreshed = true;

        sqlite3pp::database db(DB::db_path.toStdString().c_str());

        QString sql = "select s.rowid, s.name, t.count, r.count, r.wpm, "
                "nullif(t.dis,t.count) "
                "from source as s "
                "left join (select source,count(*) as count,count(disabled) "
                "as dis from text group by source) as t "
                "on (s.rowid = t.source) "
                "left join (select source,count(*) as count,avg(wpm) "
                "as wpm from result group by source) as r "
                "on (t.source = r.source) "
                "where s.disabled is null order by s.name";

        sqlite3pp::query qry(db, sql.toStdString().c_str());
        QList<QStandardItem*> items;
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                items << new QStandardItem(QString((*i).get<char const*>(0)));
                items << new QStandardItem(QString((*i).get<char const*>(1)));
                items << new QStandardItem(QString((*i).get<char const*>(2)));
                items << new QStandardItem(QString((*i).get<char const*>(3)));
                items << new QStandardItem(QString::number((*i).get<double>(4), 'f', 1));
                QString dis = ((*i).column_type(5) == SQLITE_NULL) ? "No" : "Yes";
                items << new QStandardItem(dis);

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                sourcesModel->appendRow(items);
                items.clear();
        }

        ui->sourcesTable->setModel(sourcesModel);
        ui->sourcesTable->resizeColumnsToContents();
}

void TextManager::populateTexts(const QModelIndex& index)
{
        QSettings s;
        int row = index.row();
        const QModelIndex& f = sourcesModel->index(row, 0);

        textsModel->clear();

        QStringList headers;
        headers << "id"
                << "Text"
                << "Length"
                << "Results"
                << "WPM"
                << "Dis";
        textsModel->setHorizontalHeaderLabels(headers);

        ui->textsTable->setSortingEnabled(false);
        ui->textsTable->setColumnHidden(0, true);

        QHeaderView* verticalHeader = ui->textsTable->verticalHeader();
        verticalHeader->sectionResizeMode(QHeaderView::Fixed);
        verticalHeader->setDefaultSectionSize(24);

        

        QString dir = qApp->applicationDirPath()
                + QDir::separator()
                + s.value("db_name").toString();

        sqlite3pp::database db(DB::db_path.toStdString().c_str());

        QString sql = "select t.rowid, substr(t.text,0,30)||' ...', "
                "length(t.text), r.count, r.m, t.disabled "
                "from (select rowid,* from text where source = "+f.data().toString()+") as t "
                "left join (select text_id,count(*) as count, avg(wpm) "
                "as m from result group by text_id) as r "
                "on (t.id = r.text_id) "
                "order by t.rowid";
        sqlite3pp::query qry(db, sql.toStdString().c_str());

        QList<QStandardItem*> items;

        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                items << new QStandardItem(QString((*i).get<char const*>(0)));
                items << new QStandardItem(QString((*i).get<char const*>(1)).simplified());
                items << new QStandardItem(QString((*i).get<char const*>(2)));
                items << new QStandardItem(QString((*i).get<char const*>(3)));
                items << new QStandardItem(QString::number((*i).get<double>(4), 'f', 1));
                QString dis = ((*i).column_type(5) == SQLITE_NULL) ? "." : "-";
                items << new QStandardItem(dis);

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                textsModel->appendRow(items);
                items.clear();
        }

        ui->textsTable->setModel(textsModel);
        ui->textsTable->resizeColumnsToContents();
}


void TextManager::doubleClicked(const QModelIndex& idx)
{
        int row = idx.row();
        const QModelIndex& f = textsModel->index(row, 0);

        sqlite3pp::database db(DB::db_path.toStdString().c_str());// = DB::openDB();
        QString sql = "select id, source, text from text where rowid = \""+f.data().toString()+"\"";
        sqlite3pp::query qry(db, sql.toStdString().c_str());
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                QByteArray _id     = QByteArray((*i).get<char const*>(0));
                int        _source = (*i).get<int>(1);
                QString    _text   = QString((*i).get<char const*>(2));
                Text* t = new Text(_id, _source, _text);

                emit setText(t);
                emit gotoTab(0); 
                return;
        }
}

void TextManager::nextText()
{
        QSettings s;
        Text* t = 0;

        int type = s.value("select_method").toInt();

        sqlite3pp::database db(DB::db_path.toStdString().c_str());

        if (type != 1) {
                // not in order
                QString sql = "select id,source,text from text "
                        "where disabled is null "
                        "order by random() limit "+s.value("num_rand").toString();

                sqlite3pp::query qry(db, sql.toStdString().c_str());

                for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                        if (type == 2) {
                                std::cout << 2;
                        } else if (type == 3) {
                                std::cout << 3;
                        } else {
                                QByteArray _id     = QByteArray((*i).get<char const*>(0));
                                int        _source = (*i).get<int>(1);
                                QString    _text   = QString((*i).get<char const*>(2));
                                t = new Text(_id, _source, _text);    
                                break;
                        }
                }
        } else {
                // in order
                QString sql = "select r.text_id from result as r "
                          "left join source as s on (r.source = s.rowid) "
                          "where (s.discount is null) or (s.discount = 1) "
                          "order by r.w desc limit 1";
                sqlite3pp::query qry(db, sql.toStdString().c_str());
                for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                        QByteArray g(QByteArray((*i).get<char const*>(0)));

                        QString sql2 = "select rowid from text where id = \""+QString(g)+"\"";
                        sqlite3pp::query qry2(db, sql2.toStdString().c_str());

                        for (sqlite3pp::query::iterator j = qry2.begin(); j != qry2.end(); ++j) {
                                int lastid = (*j).get<int>(0);
                                QString sql3 = "select id,source,text from text "
                                  "where rowid > "+QString::number(lastid)+" and disabled is null "
                                  "order by rowid asc limit 1";
                                sqlite3pp::query qry3(db, sql3.toStdString().c_str());

                                for (sqlite3pp::query::iterator k = qry3.begin(); k != qry3.end(); ++k) {
                                        QByteArray _id     = QByteArray((*k).get<char const*>(0));
                                        int        _source = (*k).get<int>(1);
                                        QString    _text   = QString((*k).get<char const*>(2));
                                        t = new Text(_id, _source, _text);    
                                        break;
                                }
                                break;
                        }
                        break;
                }
        }

        if (t == 0) {
                t = new Text(QByteArray(""), 0, "Welcome to Amphetype!\nA "
                        "typing program that not only measures your speed and "
                        "progress, but also gives you detailed statistics about"
                        " problem keys, words, common mistakes, and so on. This"
                        " is just a default text since your database is empty. "
                        "You might import a novel or text of your choosing and "
                        "text excerpts will be generated for you automatically."
                        " There are also some facilities to generate lessons "
                        "based on your past statistics! But for now, go to the "
                        "\"Sources\" tab and try adding some texts from the "
                        "\"txt\" directory.");
        }

        emit setText(t);
}

void TextManager::addFiles()
{
        files = QFileDialog::getOpenFileNames(
                this, tr("Import"), ".",
                tr("UTF-8 text files (*.txt);;All files (*)"));

        ui->progress->show();
        ui->progressText->show();
        lmc = new LessonMinerController;
        // lesson miner signals
        connect(lmc,  SIGNAL(workDone()),
                this, SLOT(processNextFile()));
        connect(lmc,          &LessonMinerController::progressUpdate,
                ui->progress, &QProgressBar::setValue);

        processNextFile();
}

void TextManager::processNextFile()
{
        if (files.isEmpty()) {
                refreshSources();
                ui->progressText->hide();
                ui->progress->hide();
                delete lmc;
                return;
        }
        ui->progressText->setText(files.front());
        lmc->operate(files.front());
        files.pop_front();
}
