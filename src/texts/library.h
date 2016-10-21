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
#include <QMainWindow>
#include <QModelIndex>
#include <QProgressDialog>

#include <memory>

#include "texts/lessonminercontroller.h"
#include "texts/sourcemodel.h"
#include "texts/text.h"
#include "texts/textmodel.h"

#include "defs.h"

namespace Ui {
class Library;
}

class Library : public QMainWindow {
  Q_OBJECT

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
  void reload();
  void refreshSource(int);
  void refreshSources();
  void selectSource(int source);
  void sourceSelectionChanged(const QItemSelection&, const QItemSelection&);

 private slots:
  void textsTableDoubleClickHandler(const QModelIndex&);
  void textsTableClickHandler(const QModelIndex&);
  bool validateXml(QFile* file);
  void addFiles();
  void processNextFile();
  void addSource();
  void deleteSource();
  void addText();
  void enableSource();
  void disableSource();
  void exportSource();
  void importSource();
  void actionDeleteTexts(bool checked);
  void actionEditText(bool checked);
  void actionSendToTyper(bool checked);
  void sourcesContextMenu(const QPoint& pos);
  void textsContextMenu(const QPoint& pos);

 private:
  Ui::Library* ui;
  TextModel* text_model_;
  SourceModel* source_model_;
  QProgressDialog* progress_;
  QStringList files_;
  LessonMinerController* lmc_;
};

#endif  // SRC_TEXTS_LIBRARY_H_
