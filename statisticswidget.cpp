#include "statisticswidget.h"
#include "ui_statisticswidget.h"
#include "db.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include "inc/sqlite3pp.h"

#include <vector>
#include <algorithm>
#include <string>

#include <QStandardItemModel>
#include <QSettings>
#include <QApplication>
#include <QDir>

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
        int cat = ui->typeComboBox->currentIndex();
        int limit = ui->limitEdit->text().toInt();
        int count = ui->minCountEdit->text().toInt();

        boost::posix_time::ptime history =
                boost::posix_time::microsec_clock::local_time();

        history = history - boost::posix_time::seconds(s.value("history").toInt()*86400);        

        // get the data and populate the model
        sqlite3pp::database* db = DB::openDB(s.value("db_name").toString());
        DB::addFunctions(db);

        QString query = "select data, "
                  "12.0/time as wpm, "
                  "100.0-100.0*misses/cast(total as real) as accuracy, "
                  "viscosity, "
                  "total, "
                  "misses, "
                  "total*time*time*(1.0+misses/total) as damage "
                        "from (select data, "
                               "agg_median(time) as time, "
                               "agg_median(viscosity) as viscosity, "
                               "sum(count) as total, "
                               "sum(mistakes) as misses "
                               "from statistic "
                               "where datetime(w) >= datetime('"+QString::fromStdString(boost::posix_time::to_iso_extended_string(history))+"') "
                                "and type = "+QString::number(cat)+" group by data) "
                "where total >= "+QString::number(count)+" "
                "order by "+ord+" limit "+QString::number(limit);

        sqlite3pp::query qry(*db, query.toStdString().c_str());

        QFont font("Monospace");
        font.setStyleHint(QFont::Monospace);

        QList<QStandardItem*> items;
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                QList<QByteArray> cols;
                for (int j = 0; j < qry.column_count(); ++j)
                        cols << (*i).get<char const*>(j);
                // data
                QString data(cols[0]);
                data.replace(" ", "␣"); // UNICODE U+2423 OPEN BOX (visible space)
                items << new QStandardItem(data);
                items.last()->setFont(font);
                // speed
                items << new QStandardItem(QString::number(cols[1].toDouble(), 'f', 1) + " wpm");
                // accuracy
                items << new QStandardItem(QString::number(cols[2].toDouble(), 'f', 1) + "%");
                // viscosity
                items << new QStandardItem(QString::number(cols[3].toDouble(), 'f', 1));
                //count
                items << new QStandardItem(QString::number(cols[4].toInt()));
                //mistakes
                items << new QStandardItem(QString::number(cols[5].toInt()));
                //impact
                items << new QStandardItem(QString::number(cols[6].toDouble(), 'f', 1));

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                model->appendRow(items);
                items.clear();
        }
        delete db;

        ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        ui->tableView->resizeColumnsToContents();       
}