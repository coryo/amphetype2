#include "textmanager.h"
#include "ui_textmanager.h"

#include "lessonminer.h"
#include "lessonminercontroller.h"
#include "db.h"
#include "text.h"

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
        ui->textsTable->hide();

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
                textsModel->clear();
                ui->textsTable->hide();
        }
}

void TextManager::refreshSources()
{
        sourcesModel->clear();

        QStringList headers;
        headers << "id"
                << "Source Name"
                << "# Items"
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

        QList<QStringList> rows = DB::getSourcesData();
        for (QStringList row : rows) {
                QList<QStandardItem*> items;
                items << new QStandardItem(row[0]);
                items << new QStandardItem(row[1]);
                items << new QStandardItem(row[2]);
                items << new QStandardItem(row[3]);
                items << new QStandardItem(QString::number(row[4].toDouble(), 'f', 1));
                QString dis = (row[5].isEmpty()) ? "Yes" : "No";
                items << new QStandardItem(dis);

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                sourcesModel->appendRow(items);
        }

        ui->sourcesTable->setModel(sourcesModel);
        ui->sourcesTable->resizeColumnsToContents();
}

void TextManager::populateTexts(const QModelIndex& index)
{
        ui->textsTable->show();
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

        QList<QStringList> rows = DB::getTextsData(f.data().toInt());
        for (QStringList row : rows) {
                QList<QStandardItem*> items;
                items << new QStandardItem(row[0]);
                items << new QStandardItem(row[1].simplified());
                items << new QStandardItem(row[2]);
                items << new QStandardItem(row[3]);
                items << new QStandardItem(QString::number(row[4].toDouble(), 'f', 1));
                QString dis = (row[5].isEmpty()) ? "Yes" : "No";
                items << new QStandardItem(dis);

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                textsModel->appendRow(items);
        }

        ui->textsTable->setModel(textsModel);
        ui->textsTable->resizeColumnsToContents();
}

void TextManager::doubleClicked(const QModelIndex& idx)
{
        int row = idx.row();
        const QModelIndex& f = textsModel->index(row, 0);

        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());

                QString sql = "select t.id, t.source, t.text, s.name, t.rowid from text as t "
                        "inner join source as s "
                        "on (t.source = s.rowid) "
                        "where t.rowid = "+ f.data().toString();

                sqlite3pp::query qry(db, sql.toStdString().c_str());

                sqlite3pp::query::iterator it = qry.begin();
                QByteArray _id = QByteArray( (*it).get<char const*>(0) );
                int _source    = (*it).get<int>(1);
                QString _text  = QString( (*it).get<char const*>(2) );
                QString _sName = QString( (*it).get<char const*>(3) );
                int _tNum      = (*it).get<int>(4);

                sql = "select rowid from text where source = "+QString::number(_source)+" limit 1";

                sqlite3pp::query qry2(db, sql.toStdString().c_str());
                int offset = 0;
                it = qry2.begin();
                offset = (*it).get<int>(0) - 1;

                Text* t = new Text(_id, _source, _text, _sName, _tNum-offset);

                emit setText(t);
                emit gotoTab(0); 
        }
        catch (std::exception& e) {
                std::cout << e.what() << std::endl;
                return;
        }

}

void TextManager::nextText()
{
        QSettings s;
        Text* t = 0;

        int type = s.value("select_method").toInt();

        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                sqlite3pp::query::iterator it;
                QString sql;

                if (type != 1) {
                        // not in order
                        sql = "SELECT t.id, t.source, t.text, s.name, t.rowid "
                                "FROM ((select id,source,text,rowid from text "
                                "where disabled is null order by random() "
                                "limit "+s.value("num_rand").toString()+") as t) "
                                        "INNER JOIN source as s "
                                "ON (t.source = s.rowid)";
                        sqlite3pp::query qry(db, sql.toStdString().c_str());

                        if (type == 2) {
                                std::cout << 2;
                        } else if (type == 3) {
                                std::cout << 3;
                        } else {
                                it = qry.begin();

                                QByteArray _id     = QByteArray((*it).get<char const*>(0));
                                int        _source = (*it).get<int>(1);
                                QString    _text   = QString((*it).get<char const*>(2));
                                QString    _sName  = QString((*it).get<char const*>(3));
                                int        _tNum   = (*it).get<int>(4);

                                sql = "select rowid from text where source = "
                                        + QString::number(_source) + " limit 1";

                                sqlite3pp::query qry2(db, sql.toStdString().c_str());

                                it = qry2.begin();
                                int offset = (*it).get<int>(0) - 1;

                                t = new Text(_id, _source, _text, _sName, _tNum-offset);    
                        }
                } else {
                        // in order

                        // first query
                        sql = "select r.text_id from result as r "
                                "left join source as s on (r.source = s.rowid) "
                                "where (s.discount is null) or (s.discount = 1) "
                                "order by r.w desc limit 1";
                        sqlite3pp::query qry(db, sql.toStdString().c_str());

                        it = qry.begin();
                        QByteArray g = QByteArray((*it).get<char const*>(0));

                        // second query
                        sql = "select rowid from text where id = \""+QString(g)+"\"";
                        sqlite3pp::query qry2(db, sql.toStdString().c_str());

                        it = qry2.begin();
                        int lastid = (*it).get<int>(0);
                        
                        // third query
                        sql = "select t.id,t.source,t.text, s.name, t.rowid from text as t "
                                "left join source as s on (t.source = s.rowid) "
                                "where t.rowid > "+QString::number(lastid)+" and t.disabled is null "
                                "order by t.rowid asc limit 1";
                        sqlite3pp::query qry3(db, sql.toStdString().c_str());

                        it = qry3.begin();
                        QByteArray _id     = QByteArray((*it).get<char const*>(0));
                        int        _source = (*it).get<int>(1);
                        QString    _text   = QString((*it).get<char const*>(2));
                        QString    _sName  = QString((*it).get<char const*>(3));
                        int        _tNum   = (*it).get<int>(4);

                        // fourth query
                        sql = "select rowid from text where source = "+QString::number(_source)+" limit 1";
                        sqlite3pp::query qry4(db, sql.toStdString().c_str());

                        it = qry4.begin();
                        int offset = (*it).get<int>(0) - 1;

                        // done
                        t = new Text(_id, _source, _text, _sName, _tNum-offset);
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
        catch (std::exception& e) {
                std::cout << "exception: " <<std::endl;
                std::cout << e.what() << std::endl;
                return;
        }
   
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
