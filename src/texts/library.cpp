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

#include "texts/library.h"

#include <QAction>
#include <QCursor>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QModelIndex>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QString>

#include <QsLog.h>

#include "database/db.h"
#include "texts/lessonminer.h"
#include "texts/lessonminercontroller.h"
#include "texts/sourcemodel.h"
#include "texts/text.h"
#include "texts/textmodel.h"
#include "ui_library.h"

Library::Library(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::Library),
      text_model_(new TextModel),
      source_model_(new SourceModel) {
  ui->setupUi(this);

  QSettings s;

  ui->sourcesTable->setModel(source_model_);
  ui->textsTable->setModel(text_model_);

  // Set resize mode on both tables, stretch 1st col, resize the rest
  auto sourceHeader = ui->sourcesTable->horizontalHeader();
  auto textHeader = ui->textsTable->horizontalHeader();
  sourceHeader->setSectionResizeMode(0, QHeaderView::Stretch);
  for (int col = 1; col < sourceHeader->count(); col++)
    sourceHeader->setSectionResizeMode(col, QHeaderView::ResizeToContents);
  textHeader->setSectionResizeMode(0, QHeaderView::Stretch);
  for (int col = 1; col < textHeader->count(); col++)
    textHeader->setSectionResizeMode(col, QHeaderView::ResizeToContents);

  connect(ui->actionImport_Texts, &QAction::triggered, this,
          &Library::addFiles);
  connect(ui->actionAdd_Source, &QAction::triggered, this, &Library::addSource);
  connect(ui->actionAdd_Text, &QAction::triggered, this, &Library::addText);

  connect(ui->textsTable, &QTableView::clicked, this,
          &Library::textsTableClickHandler);

  connect(ui->textsTable, &QTableView::doubleClicked, this,
          &Library::textsTableDoubleClickHandler);

  connect(ui->sourcesTable, &QWidget::customContextMenuRequested, this,
          &Library::sourcesContextMenu);

  connect(ui->textsTable, &QWidget::customContextMenuRequested, this,
          &Library::textsContextMenu);

  connect(ui->sourcesTable->selectionModel(),
          &QItemSelectionModel::currentRowChanged, this,
          &Library::sourceSelectionChanged);

  source_model_->refresh();
}

Library::~Library() {
  delete ui;
  delete text_model_;
  delete source_model_;
}

void Library::reload() {
  delete text_model_;
  delete source_model_;
  text_model_ = new TextModel;
  source_model_ = new SourceModel;

  ui->sourcesTable->setModel(source_model_);
  ui->textsTable->setModel(text_model_);

  connect(ui->sourcesTable->selectionModel(),
          &QItemSelectionModel::currentRowChanged, this,
          &Library::sourceSelectionChanged);
  source_model_->refresh();
}

void Library::sourceSelectionChanged(const QModelIndex& current,
                                     const QModelIndex& previous) {
  int source = current.data(Qt::UserRole).toInt();
  text_model_->setSource(source);
}

void Library::sourcesContextMenu(const QPoint& pos) {
  QMenu menu(this);

  auto selectedRows = ui->sourcesTable->selectionModel()->selectedRows();
  int selectedCount = selectedRows.size();

  QAction* deleteAction = menu.addAction("delete");
  connect(deleteAction, &QAction::triggered, this, &Library::deleteSource);

  QAction* enableAction = menu.addAction("enable");
  connect(enableAction, &QAction::triggered, this, &Library::enableSource);

  QAction* disableAction = menu.addAction("disable");
  connect(disableAction, &QAction::triggered, this, &Library::disableSource);

  menu.exec(QCursor::pos());
}

void Library::textsContextMenu(const QPoint& pos) {
  QMenu menu(this);

  auto selectedRows = ui->textsTable->selectionModel()->selectedRows();
  int selectedCount = selectedRows.size();

  if (selectedCount == 1) {
    QAction* testAction = menu.addAction("Send to Typer");
    testAction->setData(selectedRows[0].data(Qt::UserRole));
    connect(testAction, &QAction::triggered, this, &Library::actionSendToTyper);

    QAction* editAction = menu.addAction("edit");
    connect(editAction, &QAction::triggered, this, &Library::actionEditText);
  }

  QAction* deleteAction = menu.addAction("delete");
  connect(deleteAction, &QAction::triggered, this, &Library::actionDeleteTexts);

  menu.exec(QCursor::pos());
}

void Library::actionEditText(bool checked) {
  auto indexes = ui->textsTable->selectionModel()->selectedRows();
  if (!ui->textsTable->selectionModel()->hasSelection() || indexes.size() > 1)
    return;
  int id = indexes[0].data(Qt::UserRole).toInt();

  Database db;
  auto t = db.getText(id);

  bool ok;
  QString text = QInputDialog::getMultiLineText(this, tr("Edit Text:"),
                                                tr("Text:"), t->getText(), &ok);

  if (ok && !text.isEmpty()) {
    if (text == t->getText()) {
      return;
    }
    db.updateText(id, text);
    text_model_->refreshText(indexes[0]);
  }
}

void Library::actionDeleteTexts(bool checked) {
  if (!ui->textsTable->selectionModel()->hasSelection()) return;
  auto indexes = ui->textsTable->selectionModel()->selectedRows();

  QList<int> text_ids;
  for (const auto& index : indexes) {
    text_ids << index.data(Qt::UserRole).toInt();
  }
  Database db;
  db.deleteText(text_ids);

  if (ui->sourcesTable->selectionModel()->hasSelection()) {
    auto sourceIndex = ui->sourcesTable->selectionModel()->selectedRows()[0];
    source_model_->refreshSource(sourceIndex);
    emit sourceChanged(sourceIndex.data(Qt::UserRole).toInt());
  }

  text_model_->removeIndexes(indexes);
  emit textsDeleted(text_ids);
}

void Library::actionSendToTyper(bool checked) {
  auto sender = reinterpret_cast<QAction*>(this->sender());
  const QVariant& id = sender->data();

  if (!id.isValid()) return;

  QLOG_DEBUG() << id.toInt();
  Database db;
  auto t = db.getText(id.toInt());
  emit setText(t);
  emit gotoTab(0);
}

void Library::textsTableDoubleClickHandler(const QModelIndex& index) {
  QVariant text_id = index.data(Qt::UserRole);
  if (!text_id.isValid()) return;

  QLOG_DEBUG() << text_id.toInt();

  Database db;
  auto t = db.getText(text_id.toInt());
  emit setText(t);
  emit gotoTab(0);
}

void Library::textsTableClickHandler(const QModelIndex& index) {
  QLOG_DEBUG() << index.data(Qt::UserRole);
}

void Library::enableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  for (const auto& index : indexes) source_model_->enableSource(index);
}

void Library::disableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  for (const QModelIndex& index : indexes) source_model_->disableSource(index);
}

void Library::addText() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  if (indexes.isEmpty()) return;

  bool ok;
  QString text = QInputDialog::getMultiLineText(this, tr("Add Text:"),
                                                tr("Text:"), "", &ok);

  if (ok && !text.isEmpty()) {
    int source = indexes[0].data(Qt::UserRole).toInt();
    Database db;
    db.addText(source, text, -1, false);
    source_model_->refreshSource(indexes[0]);
    text_model_->refresh();
  }
}

void Library::addSource() {
  bool ok;
  QString sourceName = QInputDialog::getText(
      this, tr("Add Source:"), tr("Source name:"), QLineEdit::Normal, "", &ok);

  if (ok && !sourceName.isEmpty()) {
    source_model_->addSource(sourceName);
    emit sourcesChanged();
  }
}

void Library::deleteSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  QList<int> sources;
  for (const auto& index : indexes) sources << index.data(Qt::UserRole).toInt();

  source_model_->removeIndexes(indexes);
  emit sourcesDeleted(sources);
  emit sourcesChanged();
}

void Library::refreshSources() { source_model_->refresh(); }

void Library::refreshSource(int source) {
  source_model_->refreshSource(source);
}

void Library::selectSource(int source) {
  for (int i = 0; i < source_model_->rowCount(); i++) {
    auto index = source_model_->index(i, 0);
    if (index.data(Qt::UserRole) == source) {
      ui->sourcesTable->setCurrentIndex(index);
      return;
    }
  }
}

void Library::nextText(const std::shared_ptr<Text>& lastText,
                       Amphetype::SelectionMethod method) {
  QSettings s;
  Amphetype::SelectionMethod selectMethod;
  if (method != Amphetype::SelectionMethod::None) {
    selectMethod = static_cast<Amphetype::SelectionMethod>(method);
  } else {
    if (lastText && lastText->getType() > 0)
      selectMethod = Amphetype::SelectionMethod::InOrder;
    else
      selectMethod = static_cast<Amphetype::SelectionMethod>(
          s.value("select_method", 0).toInt());
  }

  QLOG_DEBUG() << "Library::nextText"
               << "select_method =" << static_cast<int>(selectMethod);

  std::shared_ptr<Text> nextText;
  Database db;
  switch (selectMethod) {
    case Amphetype::SelectionMethod::Random:
    case Amphetype::SelectionMethod::Ask:
      QLOG_DEBUG() << "nextText: Random";
      nextText = db.getRandomText();
      emit setText(nextText);
      break;
    case Amphetype::SelectionMethod::InOrder:
      QLOG_DEBUG() << "nextText: In Order";
      if (lastText) {
        nextText = db.getNextText(lastText);
        emit setText(nextText);
      } else {
        nextText = db.getNextText();
        emit setText(nextText);
      }
      break;
    case Amphetype::SelectionMethod::Repeat:
      QLOG_DEBUG() << "nextText: Repeat";
      emit setText(lastText);
      return;
  }
}

void Library::addFiles() {
  files = QFileDialog::getOpenFileNames(
      this, tr("Import"), ".", tr("UTF-8 text files (*.txt);;All files (*)"));
  if (files.isEmpty()) return;

  lmc = new LessonMinerController;
  connect(lmc, SIGNAL(workDone()), this, SLOT(processNextFile()));

  progress_ = new QProgressDialog("Importing...", "Abort", 0, 100);
  progress_->setMinimumDuration(0);
  progress_->setAutoClose(false);
  connect(lmc, &LessonMinerController::progressUpdate, progress_,
          &QProgressDialog::setValue);

  processNextFile();
}

void Library::processNextFile() {
  if (files.isEmpty()) {
    refreshSources();
    emit sourcesChanged();
    delete progress_;
    delete lmc;
    return;
  }
  progress_->setLabelText(files.front());
  lmc->operate(files.front());
  files.pop_front();
}
