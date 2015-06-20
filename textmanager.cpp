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

        int rowid = f.data().toInt();

        Text* t = DB::getText(rowid);

        emit setText(t);
        emit gotoTab(0);
}

void TextManager::nextText()
{
        QSettings s;
        int selectMethod = s.value("select_method").toInt();

        Text* t = DB::getNextText(selectMethod);

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
