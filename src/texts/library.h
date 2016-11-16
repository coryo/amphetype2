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

#ifndef SRC_TEXTS_LIBRARY_H_
#define SRC_TEXTS_LIBRARY_H_

#include <QFile>
#include <QItemSelection>
#include <QList>
#include <QMainWindow>
#include <QModelIndex>

#include <memory>

#include "database/databasemodel.h"
#include "database/db.h"
#include "texts/text.h"
#include "defs.h"

namespace Ui {
class Library;
}

class Library : public QMainWindow, public AmphetypeWindow {
  Q_OBJECT
  Q_INTERFACES(AmphetypeWindow)

 public:
  explicit Library(QWidget* parent = Q_NULLPTR);
  ~Library();

 signals:
  void setText(const std::shared_ptr<Text>&);
  void progress(int);
  void sourceDeleted(int);
  void sourcesChanged();
  void sourceChanged(int);
  void sourcesDeleted(const QList<int>&);
  void textsChanged(const QList<int>&);
  void textsDeleted(const QList<int>&);

 public slots:
  void onProfileChange() override;
  void refreshSource(int);
  void refreshSources();
  void selectSource(int source);
  void sourceSelectionChanged(const QItemSelection&, const QItemSelection&);

 private slots:
  bool validateXml(QFile* file);
  void addFiles();
  void addText();
  void addSource();
  void exportSource();
  void importSource();
  void sourcesContextMenu(const QPoint& pos);
  void textsContextMenu(const QPoint& pos);

 private:
  std::unique_ptr<Ui::Library> ui;
  std::unique_ptr<Database> db_;
  std::unique_ptr<DatabaseModel> db_source_model_;
  std::unique_ptr<TextPagedDatabaseModel> db_text_model_;
};

#endif  // SRC_TEXTS_LIBRARY_H_
