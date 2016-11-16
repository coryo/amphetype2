// Copyright (C) 2016  Cory Parsons
//
// This file is part of amphetype2.
//
// amphetype2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// amphetype2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with amphetype2.  If not, see <http://www.gnu.org/licenses/>.
//

#include "analysis/statisticswidget.h"

#include <QDateTime>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringList>

#include <algorithm>
#include <string>
#include <vector>

#include <QsLog.h>

#include "database/db.h"
#include "ui_statisticswidget.h"

StatisticsWidget::StatisticsWidget(QWidget* parent)
    : QMainWindow(parent),
      ui(std::make_unique<Ui::StatisticsWidget>()),
      model_(std::make_unique<QStandardItemModel>()) {
  ui->setupUi(this);

  loadSettings();

  model_->setHorizontalHeaderLabels(QStringList() << "Item"
                                                  << "WPM"
                                                  << "Accuracy"
                                                  << "Viscosity"
                                                  << "Count"
                                                  << "Mistakes"
                                                  << "Impact");
  model_->horizontalHeaderItem(1)->setToolTip("Median");
  model_->horizontalHeaderItem(3)->setToolTip("Median");
  ui->tableView->setModel(model_.get());
  ui->tableView->setSortingEnabled(false);

  auto header = ui->tableView->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  for (int i = 1; i <= 6; ++i)
    header->setSectionResizeMode(i, QHeaderView::ResizeToContents);

  connect(ui->orderComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(populateStatistics()));
  connect(ui->typeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(populateStatistics()));
  connect(ui->limitSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(populateStatistics()));
  connect(ui->minCountSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(populateStatistics()));
  connect(ui->generatorButton, &QPushButton::pressed, this,
          &StatisticsWidget::generateList);

  connect(ui->actionCloseWindow, &QAction::triggered, this, &QWidget::close);

  connect(ui->tableView, &QWidget::customContextMenuRequested, this,
          &StatisticsWidget::contextMenu);
}

StatisticsWidget::~StatisticsWidget() {}

void StatisticsWidget::onProfileChange() {
  db_.reset(new Database);
  populateStatistics();
}

void StatisticsWidget::loadSettings() {
  QSettings s;
  ui->limitSpinBox->setValue(s.value("statisticsWidget/limit", 30).toInt());
  ui->minCountSpinBox->setValue(s.value("statisticsWidget/min", 1).toInt());
  ui->orderComboBox->setCurrentIndex(
      s.value("statisticsWidget/order", 0).toInt());
  ui->typeComboBox->setCurrentIndex(
      s.value("statisticsWidget/type", 0).toInt());
  history_ = s.value("statisticsWidget/days", 30).toInt();
}

void StatisticsWidget::saveSettings() {
  QSettings s;
  s.setValue("statisticsWidget/order", ui->orderComboBox->currentIndex());
  s.setValue("statisticsWidget/type", ui->typeComboBox->currentIndex());
  s.setValue("statisticsWidget/limit", ui->limitSpinBox->value());
  s.setValue("statisticsWidget/min", ui->minCountSpinBox->value());
  s.setValue("statisticsWidget/days", history_);
}

//void StatisticsWidget::update() { populateStatistics(); }

void StatisticsWidget::contextMenu(const QPoint& pos) {
  auto index = ui->tableView->indexAt(pos);
  QMenu menu(this);
  QAction* deleteAction = menu.addAction("delete");
  connect(deleteAction, &QAction::triggered, this,
          &StatisticsWidget::deleteItem);
  menu.exec(QCursor::pos());
}

void StatisticsWidget::deleteItem() {
  auto rows = ui->tableView->selectionModel()->selectedRows();
  //Database db;
  for (const auto& row : rows) {
    auto data = row.data();
    db_->deleteStatistic(data.toString());
  }
  populateStatistics();
}

void StatisticsWidget::generateList() {
  if (!model_->rowCount()) return;

  QStringList list;
  for (int i = 0; i < model_->rowCount(); i++) {
    auto str = model_->index(i, 0).data().toString();
    list << str;
  }
  emit newItems(list);
}

void StatisticsWidget::populateStatistics() {
  model_->removeRows(0, model_->rowCount());

  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);

  auto rows = db_->getStatisticsData(
      QDateTime::currentDateTime().addDays(-history_).toString(Qt::ISODate),
      static_cast<amphetype::statistics::Type>(
          ui->typeComboBox->currentIndex()),
      ui->minCountSpinBox->value(), static_cast<amphetype::statistics::Order>(
                                        ui->orderComboBox->currentIndex()),
      ui->limitSpinBox->value());
  for (const auto& row : rows) {
    QList<QStandardItem*> items;
    // item: key/trigram/word
    QString data(row[0].toString());
    data.replace(" ", "␣");   // UNICODE U+2423 'OPEN BOX'
    data.replace('\n', "↵");  // UNICODE U+23CE 'RETURN SYMBOL'
    items << new QStandardItem(data);
    items.last()->setFont(font);
    // speed
    items << new QStandardItem(QString::number(row[1].toDouble(), 'f', 1));
    // accuracy
    items << new QStandardItem(QString::number(row[2].toDouble(), 'f', 1) +
                               "%");
    // viscosity
    items << new QStandardItem(QString::number(row[3].toDouble(), 'f', 1));
    // count
    items << new QStandardItem(row[4].toString());
    // mistakes
    items << new QStandardItem(row[5].toString());
    // impact
    items << new QStandardItem(QString::number(row[6].toDouble(), 'f', 1));

    model_->appendRow(items);
  }
}
