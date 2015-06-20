#include "statisticswidget.h"
#include "ui_statisticswidget.h"
#include "db.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include <vector>
#include <algorithm>
#include <string>

#include <QStandardItemModel>
#include <QSettings>

namespace bpt = boost::posix_time;

StatisticsWidget::StatisticsWidget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::StatisticsWidget),
        model(new QStandardItemModel)
{
        ui->setupUi(this);

        QSettings s;

        ui->orderComboBox->setItemData(0, "wpm asc");
        ui->orderComboBox->setItemData(1, "wpm desc");
        ui->orderComboBox->setItemData(2, "viscosity desc");
        ui->orderComboBox->setItemData(3, "viscosity asc");
        ui->orderComboBox->setItemData(4, "accuracy asc");
        ui->orderComboBox->setItemData(5, "misses desc");
        ui->orderComboBox->setItemData(6, "total desc");
        ui->orderComboBox->setItemData(7, "damage desc");

        ui->limitEdit->setText(s.value("ana_many").toString());
        ui->minCountEdit->setText(s.value("ana_count").toString());

        ui->orderComboBox->setCurrentIndex(ui->orderComboBox->findData(s.value("ana_which")));
        ui->typeComboBox->setCurrentIndex(s.value("ana_what").toInt());

        connect(ui->orderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(writeSettings()));
        connect(ui->typeComboBox,  SIGNAL(currentIndexChanged(int)), this, SLOT(writeSettings()));
        connect(ui->updateButton,  SIGNAL(pressed()),                this, SLOT(writeSettings()));
}

StatisticsWidget::~StatisticsWidget()
{
        delete ui;
        delete model;
}

void StatisticsWidget::writeSettings()
{
        QSettings s;
        s.setValue("ana_which", ui->orderComboBox->itemData(ui->orderComboBox->currentIndex()).toString());
        s.setValue("ana_what", ui->typeComboBox->currentIndex());
        s.setValue("ana_many", ui->limitEdit->text().toInt());
        s.setValue("ana_count", ui->minCountEdit->text().toInt());
        refreshStatistics();
}

void StatisticsWidget::refreshStatistics()
{
        QSettings s;

        model->clear();

        QStringList headers;
        headers << "Item"
                << "Speed"
                << "Accuracy"
                << "Viscosity"
                << "Count"
                << "Mistakes"
                << "Impact";
        model->setHorizontalHeaderLabels(headers);
        ui->tableView->setModel(model);
        ui->tableView->setSortingEnabled(false);

        QHeaderView* verticalHeader = ui->tableView->verticalHeader();
        verticalHeader->sectionResizeMode(QHeaderView::Fixed);
        verticalHeader->setDefaultSectionSize(24);

        QString ord = ui->orderComboBox->itemData(ui->orderComboBox->currentIndex()).toString();
        int cat     = ui->typeComboBox->currentIndex();
        int limit   = ui->limitEdit->text().toInt();
        int count   = ui->minCountEdit->text().toInt();

        bpt::ptime history = bpt::microsec_clock::local_time();
        history = history - bpt::seconds(s.value("history").toInt()*86400);   
        QString historyString = QString::fromStdString(bpt::to_iso_extended_string(history));

        QFont font("Monospace");
        font.setStyleHint(QFont::Monospace);

        QList<QStringList> rows = DB::getStatisticsData(historyString, cat, count, ord, limit);
        for (QStringList row : rows) {
                QList<QStandardItem*> items;
                // item: key/trigram/word
                QString data(row[0]);
                data.replace(" ",  "␣"); // UNICODE U+2423 OPEN BOX (visible space)
                data.replace('\n', "↵"); // return symbol
                items << new QStandardItem(data);
                items.last()->setFont(font);
                // speed
                items << new QStandardItem(QString::number(row[1].toDouble(), 'f', 1) + " wpm");
                // accuracy
                items << new QStandardItem(QString::number(row[2].toDouble(), 'f', 1) + "%");
                // viscosity
                items << new QStandardItem(QString::number(row[3].toDouble(), 'f', 1));
                //count
                items << new QStandardItem(row[4]);
                //mistakes
                items << new QStandardItem(row[5]);
                //impact
                items << new QStandardItem(QString::number(row[6].toDouble(), 'f', 1));     

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                model->appendRow(items);           
        }

        ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        ui->tableView->resizeColumnsToContents();       
}