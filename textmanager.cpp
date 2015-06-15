#include "textmanager.h"
#include "ui_textmanager.h"

#include "lessonminer.h"
#include "lessonminercontroller.h"
#include "db.h"
#include "treemodel.h"
#include "text.h"

#include <iostream>

#include <QString>
#include <QFileDialog>
#include <QtSql>
#include <QCryptographicHash>
#include <QByteArray>
#include <QSettings>
#include <QFile>
#include <QProgressBar>
#include <QModelIndex>

TextManager::TextManager(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::TextManager),
        model(0),
        refreshed(false)
{
        ui->setupUi(this);

        QSettings s;

        QList<QVariant> rootData;
        rootData << "id"
                 << "Source"
                 << "Length"
                 << "Results"
                 << "WPM"
                 << "Enabled";
        model = new TreeModel("", rootData);

        // progress bar/text setup
        ui->progress->setRange(0,100);
        ui->progress->hide();
        ui->progressText->hide();

        ui->selectionMethod->setCurrentIndex(s.value("select_method").toInt());

        connect(ui->importButton, SIGNAL(clicked()),
                this,             SLOT(addFiles()));
        // source tree signals
        connect(ui->refreshSources, SIGNAL(clicked()),
                this,               SLOT(refreshSources()));
        connect(ui->sourcesView, SIGNAL(doubleClicked(const QModelIndex&)),
                this,            SLOT(doubleClicked(const QModelIndex&)));
        connect(ui->sourcesView, SIGNAL(expanded(QModelIndex)),
                this,            SLOT(resizeColumns()));
        connect(ui->sourcesView, SIGNAL(collapsed(QModelIndex)),
                this,            SLOT(resizeColumns()));
        connect(ui->sourcesView, SIGNAL(expanded(const QModelIndex&)),
                this,            SLOT(populateTexts(const QModelIndex&)));

        connect(ui->selectionMethod, SIGNAL(currentIndexChanged(int)),
                this,                SLOT(changeSelectMethod(int)));      
}

TextManager::~TextManager()
{
        delete ui;
        delete model;
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
        if (i != 1 && refreshed) {
                clearModel();     
                refreshed = false;   
        }

}

void TextManager::clearModel()
{
        if (model != 0)
                delete model;
        QList<QVariant> rootData;
        rootData << "id"
                 << "Source"
                 << "Length"
                 << "Results"
                 << "WPM"
                 << "Enabled";
        model = new TreeModel("", rootData);

        ui->sourcesView->setModel(model);    
}

void TextManager::refreshSources()
{
        refreshed = true;

        QSqlQuery q;
        q.prepare("select s.rowid, s.name, t.count, r.count, r.wpm, "
                  "nullif(t.dis,t.count) "
                  "from source as s "
                  "left join (select source,count(*) as count,count(disabled) "
                  "as dis from text group by source) as t "
                  "on (s.rowid = t.source) "
                  "left join (select source,count(*) as count,avg(wpm) "
                  "as wpm from result group by source) as r "
                  "on (t.source = r.source) "
                  "where s.disabled is null order by s.name");
        q.exec();
        
        QString l;  // the model is given a formatted string.
        while (q.next()) {
                QString dis = (q.value(5).isNull()) ? "No" : "Yes";
                // id
                l.append(q.value(0).toString());
                // name
                l.append("\t" + q.value(1).toString() + " ");
                // count
                l.append("\t" + q.value(2).toString() + " ");
                // result count
                l.append("\t" + q.value(3).toString() + " ");
                // wpm
                double wpm = q.value(4).toDouble();
                l.append("\t" + QString::number(wpm, 'f', 1) + " ");
                // disabled
                l.append("\t" + dis + "\n");
                // add a dummy item so the source is expandable
                l.append("    " + QString(".") + "\n");
        }

        if (l != QString::null) {
                if (model != 0)
                        delete model;
                QList<QVariant> rootData;
                rootData << "id"
                         << "Source"
                         << "Length"
                         << "Results"
                         << "WPM"
                         << "Enabled";
                model = new TreeModel(l, rootData);

                ui->sourcesView->setModel(model);
                resizeColumns();
        }
}

void TextManager::populateTexts(const QModelIndex& index)
{
        // if the source only has 1 child (the dummy item)
        if (model->rowCount(index) == 1) {
                QSqlQuery q;
                q.prepare(
                        "select t.rowid, substr(t.text,0,40)||' ...', "
                        "length(t.text), r.count, r.m, t.disabled "
                        "from (select rowid,* from text where source = ?) as t "
                        "left join (select text_id,count(*) as count, avg(wpm) "
                        "as m from result group by text_id) as r "
                        "on (t.id = r.text_id) "
                        "order by t.rowid");
                q.addBindValue(model->data(index,Qt::DisplayRole).toInt());
                q.exec();

                // each text in the source
                QList<QStringList> data;
                while(q.next()) {
                        QStringList item;
                        QString dis = (q.value(5).isNull()) ? "" : "-";
                        item << q.value(0).toString();
                        item << q.value(1).toString().simplified();
                        item << q.value(2).toString();
                        item << q.value(3).toString();
                        double wpm = q.value(4).toDouble();
                        item << QString::number(wpm, 'f', 1);
                        item << dis;
                        data << item;
                }
                model->populateData(index, data); 

                resizeColumns();

                // collapse and expand it, to show the new data
                // block signal so it doesnt loop this slot
                ui->sourcesView->collapse(index);
                ui->sourcesView->blockSignals(true);
                ui->sourcesView->expand(index);
                ui->sourcesView->blockSignals(false);
        }

}

void TextManager::resizeColumns()
{
        ui->sourcesView->resizeColumnToContents(0);
        ui->sourcesView->resizeColumnToContents(1);
        ui->sourcesView->resizeColumnToContents(2);
        ui->sourcesView->resizeColumnToContents(3);
        ui->sourcesView->resizeColumnToContents(4);
}

void TextManager::doubleClicked(const QModelIndex& idx)
{
        model->data(idx, Qt::DisplayRole);

        QSqlQuery q;
        q.prepare("select id, source, text from text where rowid = :textrow");
        q.bindValue(":textrow", model->getId(idx, Qt::DisplayRole));
        q.exec();
        q.first();

        QByteArray _id     = q.value(0).toByteArray();
        int        _source = q.value(1).toInt();
        QString    _text   = q.value(2).toString();
        Text* t = new Text(_id, _source, _text);

        emit setText(t);
        emit gotoTab(0);
}

void TextManager::nextText()
{
        QSettings s;
        Text* t = 0;

        int type = s.value("select_method").toInt();

        QSqlQuery q;
        if (type != 1) {
                // not in order
                q.prepare("select id,source,text from text "
                        "where disabled is null "
                        "order by random() limit ?");
                q.addBindValue(s.value("num_rand"));
                q.exec();
                q.first();
                if (!q.isValid()) {
                        t = 0;
                } else if (type ==2) {
                        //
                } else if (type == 3) {
                        //
                } else {
                        QByteArray _id     = q.value(0).toByteArray();
                        int        _source = q.value(1).toInt();
                        QString    _text   = q.value(2).toString();
                        t = new Text(_id, _source, _text);
                }
        } else {
                // in order
                q.prepare("select r.text_id from result as r "
                          "left join source as s on (r.source = s.rowid) "
                          "where (s.discount is null) or (s.discount = 1) "
                          "order by r.w desc limit 1");
                q.exec();
                if (q.next()) {
                        QByteArray g(q.value(0).toByteArray());

                        q.prepare("select rowid from text where id = :lastid");
                        q.bindValue(":lastid", g);
                        q.exec();
                        q.first();
                        int lastid = q.value(0).toInt();

                        q.prepare("select id,source,text from text "
                                  "where rowid > :lastid and disabled is null "
                                  "order by rowid asc limit 1");
                        q.bindValue(":lastid", lastid);
                        q.exec();
                        q.first();

                        QByteArray _id     = q.value(0).toByteArray();
                        int        _source = q.value(1).toInt();
                        QString    _text   = q.value(2).toString();
                        t = new Text(_id, _source, _text);
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
