#include "texts/textmanager.h"

#include <QString>
#include <QFileDialog>
#include <QSettings>
#include <QFile>
#include <QProgressBar>
#include <QModelIndex>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QCursor>

#include <QsLog.h>

#include "ui_textmanager.h"
#include "texts/textmodel.h"
#include "texts/sourcemodel.h"
#include "texts/lessonminer.h"
#include "texts/lessonminercontroller.h"
#include "database/db.h"
#include "texts/text.h"

TextManager::TextManager(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::TextManager),
      text_model_(new TextModel),
      source_model_(new SourceModel) {
  ui->setupUi(this);

  QSettings s;

  ui->sourcesTable->setModel(source_model_);
  ui->textsTable->setModel(text_model_);

  // progress bar/text setup
  ui->progress->setRange(0, 100);
  ui->progress->hide();
  ui->progressText->hide();

  ui->selectionMethod->setCurrentIndex(s.value("select_method").toInt());

  connect(ui->importButton, SIGNAL(clicked()), this, SLOT(addFiles()));
  connect(ui->refreshSources, SIGNAL(clicked()), this, SLOT(refreshSources()));
  connect(ui->addSourceButton, SIGNAL(clicked()), this, SLOT(addSource()));
  connect(ui->addTextButton, SIGNAL(clicked()), this, SLOT(addText()));
  connect(ui->selectionMethod, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeSelectMethod(int)));

  connect(ui->textsTable, &QTableView::clicked, this,
          &TextManager::textsTableClickHandler);

  connect(ui->textsTable, &QTableView::doubleClicked, this,
          &TextManager::textsTableDoubleClickHandler);

  connect(ui->sourcesTable, &QWidget::customContextMenuRequested, this,
          &TextManager::sourcesContextMenu);

  connect(ui->textsTable, &QWidget::customContextMenuRequested, this,
          &TextManager::textsContextMenu);

  connect(text_model_, &QAbstractItemModel::rowsInserted, ui->textsTable,
          &QTableView::resizeColumnsToContents);

  connect(ui->sourcesTable->selectionModel(),
          &QItemSelectionModel::currentRowChanged, this,
          &TextManager::sourceSelectionChanged);

  this->refreshSources();
}

TextManager::~TextManager() {
  delete ui;
  delete text_model_;
  delete source_model_;
}

void TextManager::sourceSelectionChanged(const QModelIndex& current,
                                         const QModelIndex& previous) {
  int source = current.data(Qt::UserRole).toInt();
  text_model_->setSource(source);
}

void TextManager::sourcesContextMenu(const QPoint& pos) {
  QMenu menu(this);

  auto selectedRows = ui->sourcesTable->selectionModel()->selectedRows();
  int selectedCount = selectedRows.size();

  QAction* deleteAction = menu.addAction("delete");
  connect(deleteAction, &QAction::triggered, this, &TextManager::deleteSource);

  QAction* enableAction = menu.addAction("enable");
  connect(enableAction, &QAction::triggered, this, &TextManager::enableSource);

  QAction* disableAction = menu.addAction("disable");
  connect(disableAction, &QAction::triggered, this,
          &TextManager::disableSource);

  menu.exec(QCursor::pos());
}

void TextManager::textsContextMenu(const QPoint& pos) {
  QMenu menu(this);

  auto selectedRows = ui->textsTable->selectionModel()->selectedRows();
  int selectedCount = selectedRows.size();

  if (selectedCount == 1) {
    QAction* testAction = menu.addAction("Send to Typer");
    testAction->setData(selectedRows[0].data(Qt::UserRole));
    connect(testAction, &QAction::triggered, this,
            &TextManager::actionSendToTyper);

    QAction* editAction = menu.addAction("edit");
    editAction->setData(selectedRows[0].data(Qt::UserRole));
    connect(editAction, &QAction::triggered, this,
            &TextManager::actionEditText);
  }

  QAction* deleteAction = menu.addAction("delete");
  connect(deleteAction, &QAction::triggered, this,
          &TextManager::actionDeleteTexts);

  menu.exec(QCursor::pos());
}

void TextManager::actionEditText(bool checked) {
  auto sender = reinterpret_cast<QAction*>(this->sender());
  int id = sender->data().toInt();

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
  }
}

void TextManager::actionDeleteTexts(bool checked) {
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
  }
  text_model_->removeIndexes(indexes);
}

void TextManager::actionSendToTyper(bool checked) {
  auto sender = reinterpret_cast<QAction*>(this->sender());
  const QVariant& id = sender->data();

  if (!id.isValid()) return;

  QLOG_DEBUG() << id.toInt();
  Database db;
  auto t = db.getText(id.toInt());
  emit setText(t);
  emit gotoTab(0);
}

void TextManager::textsTableDoubleClickHandler(const QModelIndex& index) {
  QVariant text_id = index.data(Qt::UserRole);
  if (!text_id.isValid()) return;

  QLOG_DEBUG() << text_id.toInt();

  Database db;
  auto t = db.getText(text_id.toInt());
  emit setText(t);
  emit gotoTab(0);
}

void TextManager::textsTableClickHandler(const QModelIndex& index) {
  QLOG_DEBUG() << index.data(Qt::UserRole);
}

void TextManager::enableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  for (const auto& index : indexes) source_model_->enableSource(index);
}

void TextManager::disableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  for (const QModelIndex& index : indexes) source_model_->disableSource(index);
}

void TextManager::addText() {
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
  }
}

void TextManager::addSource() {
  bool ok;
  QString sourceName = QInputDialog::getText(
      this, tr("Add Source:"), tr("Source name:"), QLineEdit::Normal, "", &ok);

  if (ok && !sourceName.isEmpty()) {
    source_model_->addSource(sourceName);
  }
}

void TextManager::deleteSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  source_model_->removeIndexes(indexes);
  emit sourcesChanged();
}

void TextManager::changeSelectMethod(int i) {
  QSettings s;
  if (s.value("select_method").toInt() != i) s.setValue("select_method", i);
}

void TextManager::refreshSources() {
  source_model_->refresh();
  ui->sourcesTable->resizeColumnsToContents();
}

void TextManager::refreshSource(int source) {
  source_model_->refreshSource(source);
}

void TextManager::nextText(const std::shared_ptr<Text>& lastText,
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
          s.value("select_method").toInt());
  }

  QLOG_DEBUG() << "TextManager::nextText"
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
      if (!s.value("perf_logging").toBool() && lastText) {
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

void TextManager::addFiles() {
  files = QFileDialog::getOpenFileNames(
      this, tr("Import"), ".", tr("UTF-8 text files (*.txt);;All files (*)"));
  if (files.isEmpty()) return;

  ui->progress->show();
  ui->progressText->show();
  lmc = new LessonMinerController;
  // lesson miner signals
  connect(lmc, SIGNAL(workDone()), this, SLOT(processNextFile()));
  connect(lmc, &LessonMinerController::progressUpdate, ui->progress,
          &QProgressBar::setValue);

  processNextFile();
}

void TextManager::processNextFile() {
  if (files.isEmpty()) {
    refreshSources();
    ui->progressText->hide();
    ui->progress->hide();
    delete lmc;
    return;
  }
  ui->progressText->setText(files.front());
  lmc->operate(files.front());
  files.pop_front();
}
