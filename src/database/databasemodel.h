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

#ifndef SRC_DATABASE_DATABASEMODEL_H_
#define SRC_DATABASE_DATABASEMODEL_H_

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QDebug>
#include <QDialog>
#include <QItemDelegate>
#include <QList>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QtGlobal>

#include "database/db.h"

using std::vector;

class DatabaseItem {
 public:
  explicit DatabaseItem(const vector<QVariant>& row);
  const vector<QVariant>& data() const;
  void setData(const vector<QVariant>& data);

 private:
  vector<QVariant> data_;
};

class DatabaseModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  /*! `table` and `tableView` must both exist in the db. A column is editable if
    `columnName` exists in `table` and `columnName_editable` exits in
    `tableView`. */
  DatabaseModel(const QString& table, const QString& where = QString(),
                QObject* parent = Q_NULLPTR);
  ~DatabaseModel();
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  virtual void clear();
  //! remove the given row numbers from the model AND from the database
  void setWhere(const QString& where);
  void rowAdded();
  void removeRows(QList<int>& rows);
  void removeIndexes(const QModelIndexList& indexes);
  const QString primaryKey(const QModelIndex& index) const;
  void deleteIndex(const QModelIndex& index);
  void refreshAll();
  void refreshIndex(const QModelIndex& index);
  void refreshIndexes(const QModelIndexList& indexes);
  void refreshItem(const QPair<QString, QVariant>& key);
  //! populate the model with all rows
  void populate();
  void setHorizontalHeaderLabels(const QStringList& labels);

 protected:
  QString table_;
  QString where_;
  Database db_;
  QList<DatabaseItem> items_;

 private:
  QStringList header_labels_;
  QMap<QString, QVariantList> table_info_;
  QMap<QString, QVariantList> view_info_;
};

class PagedDatabaseModel : public DatabaseModel {
 public:
  PagedDatabaseModel(const QString& table, int page_size = 20,
                     const QString& where = QString(),
                     QObject* parent = Q_NULLPTR);
  bool canFetchMore(const QModelIndex& parent) const override;
  void fetchMore(const QModelIndex& parent) override;
  void clear() override;
  void setPageSize(int size);

 protected:
  //! return the row count of the table
  virtual int getTotalSize() const;

 private:
  int total_size_ = 0;
  int page_size_ = 20;
  int current_page_ = 0;
};

class TextPagedDatabaseModel : public PagedDatabaseModel {
 public:
  TextPagedDatabaseModel(const QString& table, int source, int page_size = 20,
                         QObject* parent = Q_NULLPTR);
  void setSource(int source);
  int source() const { return source_; }

 protected:
  int getTotalSize() const override;

 private:
  int source_;
};

class DatabaseItemDelegate : public QItemDelegate {
  Q_OBJECT

 public:
  DatabaseItemDelegate(QObject* parent = Q_NULLPTR);
  void commit();
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override;
};

#endif  // SRC_DATABASE_DATABASEMODEL_H_
