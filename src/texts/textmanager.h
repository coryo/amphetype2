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

#ifndef SRC_TEXTS_TEXTMANAGER_H_
#define SRC_TEXTS_TEXTMANAGER_H_

#include <QModelIndex>
#include <QResizeEvent>
#include <QWidget>


#include <memory>

#include "texts/lessonminercontroller.h"
#include "texts/sourcemodel.h"
#include "texts/text.h"
#include "texts/textmodel.h"

#include "defs.h"

namespace Ui {
class TextManager;
}

class TextManager : public QWidget {
  Q_OBJECT

 public:
  explicit TextManager(QWidget* parent = Q_NULLPTR);
  ~TextManager();

 private:
  Ui::TextManager* ui;
  TextModel* text_model_;
  SourceModel* source_model_;
  QStringList files;
  LessonMinerController* lmc;
  void textsTableDoubleClickHandler(const QModelIndex&);
  void textsTableClickHandler(const QModelIndex&);

 signals:
  void setText(const std::shared_ptr<Text>&);
  void progress(int);
  void gotoTab(int);
  void sourceDeleted(int);
  void sourcesChanged();
  void sourceChanged(int);

 public slots:
  void nextText(
      const std::shared_ptr<Text>&,
      Amphetype::SelectionMethod method = Amphetype::SelectionMethod::None);
  void refreshSource(int);
  void refreshSources();
  void sourceSelectionChanged(const QModelIndex&, const QModelIndex&);

 private slots:
  void addFiles();
  void processNextFile();
  void addSource();
  void deleteSource();
  void addText();
  void enableSource();
  void disableSource();

  void actionDeleteTexts(bool checked);
  void actionEditText(bool checked);
  void actionSendToTyper(bool checked);

  void sourcesContextMenu(const QPoint& pos);
  void textsContextMenu(const QPoint& pos);
};

#endif  // SRC_TEXTS_TEXTMANAGER_H_
