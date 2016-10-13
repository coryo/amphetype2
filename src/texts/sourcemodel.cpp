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

#include "texts/sourcemodel.h"

#include <QsLog.h>

#include "database/db.h"

SourceItem::SourceItem(Database* db, int id, const QString& name, int disabled,
                       int discount, int type, int count, int results,
                       double wpm)
    : db_(db),
      id_(id),
      name_(name),
      disabled_(disabled),
      discount_(discount),
      type_(type),
      text_count_(count),
      results_(results),
      wpm_(wpm) {}

void SourceItem::refresh() {
  auto data = db_->getSourceData(id_);
  id_ = data[0].toInt();
  name_ = data[1].toString();
  disabled_ = data[5].isNull() ? 0 : 1;
  discount_ = 0;
  type_ = 0;
  text_count_ = data[2].toInt();
  results_ = data[3].toInt();
  wpm_ = data[4].toDouble();
}

void SourceItem::deleteFromDb() {
  QList<int> ids;
  ids << id_;
  db_->deleteSource(ids);
}

void SourceItem::enable() {
  QList<int> ids;
  ids << id_;
  db_->enableSource(ids);
  disabled_ = 0;
}

void SourceItem::disable() {
  QList<int> ids;
  ids << id_;
  db_->disableSource(ids);
  disabled_ = 1;
}

SourceModel::SourceModel(QObject* parent) : QAbstractTableModel(parent) {}

SourceModel::~SourceModel() {
  for (SourceItem* item : this->items) delete item;
}

QVariant SourceModel::headerData(int section, Qt::Orientation orientation,
                                 int role) const {
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      switch (section) {
        case 0:
          return QString("Name");
        case 1:
          return QString("#");
        case 2:
          return QString("results");
        case 3:
          return QString("wpm");
        case 4:
          return QString("dis");
      }
    }
  }
  return QVariant();
}

int SourceModel::rowCount(const QModelIndex& parent) const {
  return this->items.size();
}

int SourceModel::columnCount(const QModelIndex& parent) const { return 5; }

QVariant SourceModel::data(const QModelIndex& index, int role) const {
  int row = index.row();
  int column = index.column();
  SourceItem* item = this->items[row];

  if (role == Qt::DisplayRole) {
    switch (column) {
      case 0:
        return item->name_;
      case 1:
        return item->text_count_;
      case 2:
        return item->results_;
      case 3:
        return item->wpm_ ? QString::number(item->wpm_, 'f', 1) : QVariant();
      case 4:
        return item->disabled_ ? "yes" : QVariant();
    }
  } else if (role == Qt::UserRole) {
    return item->id_;
  }
  return QVariant();
}

void SourceModel::refresh() {
  this->clear();

  QList<QVariantList> rows = db_.getSourcesData();
  if (rows.isEmpty()) return;
  this->beginInsertRows(QModelIndex(), this->rowCount(),
                        this->rowCount() + rows.size() - 1);
  for (const auto& row : rows) {
    SourceItem* item =
        new SourceItem(&db_, row[0].toInt(), row[1].toString(), row[5].toInt(),
                       0, 0, row[2].toInt(), row[3].toInt(), row[4].toDouble());
    this->items.append(item);
  }
  this->endInsertRows();
}

void SourceModel::refreshSource(const QModelIndex& index) {
  if (!index.isValid()) return;

  SourceItem* item = this->items[index.row()];
  item->refresh();
  emit dataChanged(index, this->index(index.row(), this->columnCount()));
}

void SourceModel::refreshSource(int source) {
  for (int row = 0; row < this->rowCount(); row++) {
    auto index = this->index(row, 0);
    if (index.data(Qt::UserRole) == source) {
      this->items[row]->refresh();
      emit dataChanged(index, this->index(index.row(), this->columnCount()));
      return;
    }
  }
}

void SourceModel::removeIndexes(const QModelIndexList& indexes) {
  if (indexes.isEmpty()) return;

  QList<int> rows;

  for (const auto& index : indexes) rows << index.row();
  qSort(rows.begin(), rows.end());

  for (int i = rows.size() - 1; i >= 0; i--) {
    int row = rows[i];
    this->beginRemoveRows(QModelIndex(), row, row);
    SourceItem* item = this->items.takeAt(row);
    if (item) {
      item->deleteFromDb();
      delete item;
    }
    this->endRemoveRows();
  }
}

void SourceModel::clear() {
  this->beginResetModel();
  for (SourceItem* item : this->items) delete item;
  this->items.clear();
  this->endResetModel();
}

void SourceModel::deleteSource(const QModelIndex& index) {
  if (!index.isValid()) return;
  this->beginRemoveRows(QModelIndex(), index.row(), index.row());
  if (index.row() >= 0 && index.row() < this->items.size()) {
    SourceItem* item = this->items.takeAt(index.row());
    item->deleteFromDb();
    delete item;
  }
  this->endRemoveRows();
}

void SourceModel::enableSource(const QModelIndex& index) {
  if (index.row() >= 0 && index.row() < this->items.size()) {
    this->items[index.row()]->enable();
    emit dataChanged(index, this->index(index.row(), this->columnCount()));
  }
}

void SourceModel::disableSource(const QModelIndex& index) {
  if (index.row() >= 0 && index.row() < this->items.size()) {
    this->items[index.row()]->disable();
    emit dataChanged(index, this->index(index.row(), this->columnCount()));
  }
}

void SourceModel::addSource(const QString& name) {
  db_.getSource(name);
  this->refresh();
}
