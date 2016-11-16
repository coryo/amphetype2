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

#include "database/databasemodel.h"

#include <QCursor>
#include <QDebug>
#include <QDialog>
#include <QItemDelegate>
#include <QList>
#include <QModelIndex>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>

#include "database/db.h"
#include "texts/edittextdialog.h"

DatabaseItem::DatabaseItem(const vector<QVariant>& row) : data_(row) {}
const vector<QVariant>& DatabaseItem::data() const { return data_; }
void DatabaseItem::setData(const vector<QVariant>& data) { data_ = data; }

DatabaseModel::DatabaseModel(const QString& table, const QString& where,
                             QObject* parent)
    : QAbstractTableModel(parent), table_(table), where_(where) {
  table_info_ = db_.tableInfo(table);
  view_info_ = db_.tableInfo(table + "View");
}

DatabaseModel::~DatabaseModel() {}

void DatabaseModel::setWhere(const QString& where) { where_ = where; }

void DatabaseModel::rowAdded() {
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  auto row =
      db_.getOneRow(QString("SELECT * from %1View %2 ORDER BY 1 DESC LIMIT 1")
                        .arg(table_)
                        .arg(where_));
  items_ << DatabaseItem(row);
  endInsertRows();
}

void DatabaseModel::clear() {
  beginResetModel();
  items_.clear();
  endResetModel();
}

void DatabaseModel::populate() {
  clear();
  auto rows =
      db_.getRows(QString("SELECT * from %1View %2").arg(table_).arg(where_));
  if (rows.empty()) return;
  beginInsertRows(QModelIndex(), rowCount(), rowCount() + rows.size() - 1);
  for (const auto& row : rows) items_ << DatabaseItem(row);
  endInsertRows();
}

void DatabaseModel::setHorizontalHeaderLabels(const QStringList& labels) {
  if (labels.count() == columnCount()) header_labels_ = labels;
}

QVariant DatabaseModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      return header_labels_.isEmpty() ? view_info_["name"][section]
                                      : header_labels_[section];
    } else if (orientation == Qt::Vertical) {
      int first = items_[0].data()[0].toInt();
      return 1 + items_[section].data()[0].toInt() - first;
    }
  }
  return QVariant();
}

int DatabaseModel::rowCount(const QModelIndex& parent) const {
  return items_.count();
}

int DatabaseModel::columnCount(const QModelIndex& parent) const {
  return view_info_["column"].count();
}

QVariant DatabaseModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole) {
    return items_[index.row()].data()[index.column()];
  } else if (role == Qt::UserRole) {
    return items_[index.row()].data()[0];
  } else if (role == Qt::EditRole) {
    auto data = db_.getOneRow(
        QString("SELECT %1 from %2 WHERE %3")
            .arg(view_info_["name"][index.column()].toString().split(
                "_editable")[0])
            .arg(table_)
            .arg(primaryKey(index)));
    if (!data.empty()) return data[0];
  }
  return QVariant();
}

bool DatabaseModel::setData(const QModelIndex& index, const QVariant& value,
                            int role) {
  if (role == Qt::EditRole) {
    db_.bindAndRun(QString("UPDATE %1 SET %2 = ? WHERE %3")
                       .arg(table_)
                       .arg(view_info_["name"][index.column()].toString().split(
                           "_editable")[0])
                       .arg(primaryKey(index)),
                   value);
    auto row = db_.getOneRow(QString("select * from %1View where %2")
                                 .arg(table_)
                                 .arg(primaryKey(index)));
    items_[index.row()].setData(row);
    emit dataChanged(index, index);
    return true;
  } else if (role == Qt::DisplayRole) {
    auto row = db_.getOneRow(QString("select * from %1View where %2")
                                 .arg(table_)
                                 .arg(primaryKey(index)));
    items_[index.row()].setData(row);
    emit dataChanged(index, index);
    return true;
  }
  return false;
}

void DatabaseModel::deleteIndex(const QModelIndex& index) {
  QString qry("delete from %1 where %2");
  db_.bindAndRun(qry.arg(table_).arg(primaryKey(index)));
}

void DatabaseModel::refreshIndex(const QModelIndex& index) {
  auto row = db_.getOneRow(QString("select * from %1View where %2")
                               .arg(table_)
                               .arg(primaryKey(index)));
  items_[index.row()].setData(row);
  emit dataChanged(index, index);
}

void DatabaseModel::refreshIndexes(const QModelIndexList& indexes) {
  for (const auto& index : indexes) refreshIndex(index);
}

void DatabaseModel::refreshAll() {
  for (int i = 0; i < rowCount(); ++i) {
    auto idx = index(i, 0);
    if (idx.isValid()) refreshIndex(idx);
  }
}

void DatabaseModel::refreshItem(const QPair<QString, QVariant>& key) {
  int col = view_info_["name"].indexOf(key.first);
  for (int i = 0; i < items_.count(); ++i) {
    if (items_[i].data()[col] == key.second) {
      setData(index(i, 0, QModelIndex()), QVariant(), Qt::DisplayRole);
      return;
    }
  }
}

Qt::ItemFlags DatabaseModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags flags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QString col_name = view_info_["name"][index.column()].toString();
  if (col_name.endsWith("_editable") &&
      table_info_["name"].contains(col_name.split("_editable")[0]))
    flags = flags | Qt::ItemIsEditable;
  return flags;
}

void DatabaseModel::removeIndexes(const QModelIndexList& indexes) {
  QList<int> rows;
  for (const auto& index : indexes) rows << index.row();
  removeRows(rows);
}

void DatabaseModel::removeRows(QList<int>& rows) {
  // go from highest to lowest so the row numbers stay valid
  std::sort(rows.begin(), rows.end(), std::greater<int>());

  auto group_start = rows.begin();
  while (group_start != rows.end()) {
    vector<int> group;
    group.push_back(*group_start);

    auto group_end = group_start;
    ++group_end;
    while (group_end != rows.end()) {
      if (std::abs(*(group_end - 1) - *group_end) > 1) break;
      group.push_back(*group_end);
      ++group_end;
    }
    beginRemoveRows(QModelIndex(), group.back(), group.front());
    for (int row : group) {
      deleteIndex(index(row, 0));
      items_.removeAt(row);
    }
    endRemoveRows();
    group_start = group_end;
  }
}

const QString DatabaseModel::primaryKey(const QModelIndex& index) const {
  QStringList elements;
  for (int i = 0; i < table_info_["primaryKey"].count(); ++i) {
    if (table_info_["primaryKey"][i].toBool())
      elements << table_info_["name"][i].toString();
  }
  for (auto& element : elements) {
    if (view_info_["name"].contains(element)) {
      element += (" = " +
                  items_[index.row()]
                      .data()[view_info_["name"].indexOf(element)]
                      .toString());
    }
  }
  return "(" + elements.join(" and ") + ")";
}

PagedDatabaseModel::PagedDatabaseModel(const QString& table, int page_size,
                                       const QString& where, QObject* parent)
    : DatabaseModel(table, where, parent), page_size_(page_size) {
  clear();
}

int PagedDatabaseModel::getTotalSize() const {
  auto data = db_.getOneRow(
      QString("select count() FROM %1View %2").arg(table_).arg(where_));
  return data.empty() ? 0 : data[0].toInt();
}

void PagedDatabaseModel::clear() {
  current_page_ = 0;
  total_size_ = getTotalSize();
  DatabaseModel::clear();
}

void PagedDatabaseModel::setPageSize(int size) { page_size_ = size; }

bool PagedDatabaseModel::canFetchMore(const QModelIndex& parent) const {
  return page_size_ > 0 && items_.count() < total_size_;
}

void PagedDatabaseModel::fetchMore(const QModelIndex& parent) {
  auto rows = db_.getRows(QString("SELECT * from %1View %2 LIMIT ? OFFSET ?")
                              .arg(table_)
                              .arg(where_),
                          db_row{page_size_, page_size_ * current_page_});

  beginInsertRows(QModelIndex(), rowCount(), rowCount() + rows.size() - 1);
  for (const auto& row : rows) items_ << DatabaseItem(row);
  endInsertRows();

  ++current_page_;
}

TextPagedDatabaseModel::TextPagedDatabaseModel(const QString& table, int source,
                                               int page_size, QObject* parent)
    : PagedDatabaseModel(table, page_size,
                         QString("where source = %1").arg(source), parent),
      source_(source) {}

void TextPagedDatabaseModel::setSource(int source) {
  source_ = source;
  setWhere(QString("where source = %1").arg(source));
}

int TextPagedDatabaseModel::getTotalSize() const {
  auto data =
      db_.getOneRow("select text_count FROM source where id = ?", source_);
  return data.empty() ? 0 : data[0].toInt();
}

DatabaseItemDelegate::DatabaseItemDelegate(QObject* parent)
    : QItemDelegate(parent) {}
QWidget* DatabaseItemDelegate::createEditor(QWidget* parent,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {
  auto editor = new EditTextDialog(tr("Edit Text:"), parent);
  connect(editor, &QDialog::accepted, this, &DatabaseItemDelegate::commit);
  return editor;
}
void DatabaseItemDelegate::commit() {
  auto editor = qobject_cast<EditTextDialog*>(sender());
  emit commitData(editor);
  emit closeEditor(editor);
}
void DatabaseItemDelegate::setModelData(QWidget* editor,
                                        QAbstractItemModel* model,
                                        const QModelIndex& index) const {
  auto dialog = reinterpret_cast<EditTextDialog*>(editor);
  if (dialog->text() != index.data(Qt::EditRole).toString()) {
    model->setData(index, dialog->text(), Qt::EditRole);
  }
}

void DatabaseItemDelegate::setEditorData(QWidget* editor,
                                         const QModelIndex& index) const {
  auto dialog = qobject_cast<EditTextDialog*>(editor);
  dialog->setText(index.data(Qt::EditRole).toString());
}

void DatabaseItemDelegate::updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  auto new_pos = QCursor::pos() - QPoint(0, editor->sizeHint().height());
  editor->move(new_pos.x(), qMax(0, new_pos.y()));
}
