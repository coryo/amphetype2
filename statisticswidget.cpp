#include "statisticswidget.h"
#include "ui_statisticswidget.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include <QStandardItemModel>
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QSqlQuery>

StatisticsWidget::StatisticsWidget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::StatisticsWidget),
        model(new QStandardItemModel),
        s(new QSettings(qApp->applicationDirPath() + QDir::separator() +
                                  "Amphetype2.ini",
                          QSettings::IniFormat))
{
        ui->setupUi(this);

        ui->orderComboBox->setItemData(0, "wpm asc");
        ui->orderComboBox->setItemData(1, "wpm desc");
        ui->orderComboBox->setItemData(2, "viscosity desc");
        ui->orderComboBox->setItemData(3, "viscosity asc");
        ui->orderComboBox->setItemData(4, "accuracy asc");
        ui->orderComboBox->setItemData(5, "misses desc");
        ui->orderComboBox->setItemData(6, "total desc");
        ui->orderComboBox->setItemData(7, "damage desc");

        ui->limitEdit->setText(s->value("ana_many").toString());
        ui->minCountEdit->setText(s->value("ana_count").toString());

        ui->orderComboBox->setCurrentIndex(ui->orderComboBox->findData(s->value("ana_which")));
        ui->typeComboBox->setCurrentIndex(s->value("ana_what").toInt());

        connect(ui->orderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(writeSettings()));
        connect(ui->typeComboBox,  SIGNAL(currentIndexChanged(int)), this, SLOT(writeSettings()));
        connect(ui->updateButton,  SIGNAL(pressed()),                this, SLOT(writeSettings()));

}

StatisticsWidget::~StatisticsWidget()
{
        delete ui;
        delete model;
        delete s;
}

void StatisticsWidget::writeSettings()
{
        s->setValue("ana_which", ui->orderComboBox->itemData(ui->orderComboBox->currentIndex()).toString());
        s->setValue("ana_what", ui->typeComboBox->currentIndex());
        s->setValue("ana_many", ui->limitEdit->text().toInt());
        s->setValue("ana_count", ui->minCountEdit->text().toInt());
        refreshStatistics();
}

void StatisticsWidget::resizeColumns()
{
        ui->tableView->resizeColumnToContents(0);
        ui->tableView->resizeColumnToContents(1);
        ui->tableView->resizeColumnToContents(2);
        ui->tableView->resizeColumnToContents(3);
        ui->tableView->resizeColumnToContents(4);
        ui->tableView->resizeColumnToContents(5);
        ui->tableView->resizeColumnToContents(6);
}

void StatisticsWidget::refreshStatistics()
{
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

        history = history - boost::posix_time::seconds(s->value("history").toInt()*86400);        

        QSqlQuery q;
        q.prepare(QString("select data, "
                  "12.0/time as wpm, "
                  "100.0-100.0*misses/cast(total as real) as accuracy, "
                  "viscosity, "
                  "total, "
                  "misses, "
                  "total*time*time*(1.0+misses/total) as damage "
                        "from ("
                                "select data, "
                                "min(time) as time, "
                                "min(viscosity) as viscosity, "
                                "sum(count) as total, "
                                "sum(mistakes) as misses "
                                "from statistic where datetime(w) >= datetime(:when) "
                                        "and type = :type group by data) "
                "where total >= :total "
                "order by %1 limit :limit").arg(ord));
                // order is not a value so it needs to be an arg, not bound value

        q.bindValue(":when", QString::fromStdString(boost::posix_time::to_iso_extended_string(history)));
        q.bindValue(":type", cat);
        q.bindValue(":total", count);
        q.bindValue(":limit", limit);
        q.exec();

        QList<QStandardItem*> items;
        while(q.next()) {
                // item
                items << new QStandardItem(q.value(0).toString());
                // speed
                items << new QStandardItem(QString::number(q.value(1).toDouble(), 'f', 1) + QString(" wpm"));
                // accuracy
                items << new QStandardItem(QString::number(q.value(2).toDouble(), 'f', 1) + QString("%"));
                // viscosity
                items << new QStandardItem(QString::number(q.value(3).toDouble(), 'f', 1));
                //count
                items << new QStandardItem(q.value(4).toString());
                //mistakes
                items << new QStandardItem(q.value(5).toString());
                //impact
                items << new QStandardItem(QString::number(q.value(6).toDouble(), 'f', 1));

                for (QStandardItem* item : items)
                        item->setFlags(Qt::ItemIsEnabled);
                model->appendRow(items);
                items.clear();
        }
        resizeColumns();
}