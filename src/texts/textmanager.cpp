#include "texts/textmanager.h"

#include <QString>
#include <QFileDialog>
#include <QSettings>
#include <QFile>
#include <QProgressBar>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QCursor>

#include <QsLog.h>

#include "ui_textmanager.h"
#include "texts/textmodel.h"
#include "texts/lessonminer.h"
#include "texts/lessonminercontroller.h"
#include "database/db.h"
#include "texts/text.h"

TextManager::TextManager(QWidget* parent)
    : QWidget(parent), ui(new Ui::TextManager) {
  ui->setupUi(this);

  QSettings s;

  ui->sourcesTable->setModel(new QStandardItemModel);
  ui->textsTable->setModel(new TextModel);

  // progress bar/text setup
  ui->progress->setRange(0, 100);
  ui->progress->hide();
  ui->progressText->hide();

  ui->selectionMethod->setCurrentIndex(s.value("select_method").toInt());

  connect(ui->importButton, SIGNAL(clicked()), this, SLOT(addFiles()));
  connect(ui->refreshSources, SIGNAL(clicked()), this, SLOT(refreshSources()));
  connect(ui->addSourceButton, SIGNAL(clicked()), this, SLOT(addSource()));
  connect(ui->addTextButton, SIGNAL(clicked()), this, SLOT(addText()));
  connect(ui->sourcesTable, SIGNAL(pressed(const QModelIndex&)), this,
          SLOT(populateTexts(const QModelIndex&)));
  connect(ui->selectionMethod, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeSelectMethod(int)));

  connect(ui->textsTable, &QTableView::clicked, this,
          &TextManager::textsTableClickHandler);

  connect(ui->textsTable, &QTableView::doubleClicked, this,
          &TextManager::textsTableDoubleClickHandler);

  connect(this, &TextManager::sourceSelected,
          reinterpret_cast<TextModel*>(ui->textsTable->model()),
          &TextModel::setSource);

  connect(ui->sourcesTable, &QWidget::customContextMenuRequested, this,
          &TextManager::sourcesContextMenu);

  connect(ui->textsTable, &QWidget::customContextMenuRequested, this,
          &TextManager::textsContextMenu);

  connect(reinterpret_cast<TextModel*>(ui->textsTable->model()),
          &QAbstractItemModel::rowsInserted, ui->textsTable,
          &QTableView::resizeColumnsToContents);

  connect(this, &TextManager::sourceChanged, this, &TextManager::refreshSource);
  connect(this, &TextManager::sourceDeleted, this,
          &TextManager::ui_deleteSource);

  this->refreshSources();
}

TextManager::~TextManager() { delete ui; }

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
      // delete t;
      return;
    }
    db.updateText(id, text);
  }
  // delete t;
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

  reinterpret_cast<TextModel*>(ui->textsTable->model())->removeIndexes(indexes);
  emit sourceChanged(
      reinterpret_cast<TextModel*>(ui->textsTable->model())->getSource());
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
  if (indexes.isEmpty()) return;

  QList<int> sources;
  for (QModelIndex index : indexes) {
    int row = index.row();
    const QModelIndex& f =
        reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
            ->index(row, 0);
    sources << f.data().toInt();
  }
  Database db;
  db.enableSource(sources);
  // refreshSources();
  for (int source : sources) emit sourceChanged(source);
}

void TextManager::disableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();
  if (indexes.isEmpty()) return;

  QList<int> sources;
  for (QModelIndex index : indexes) {
    int row = index.row();
    const QModelIndex& f =
        reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
            ->index(row, 0);
    sources << f.data().toInt();
  }
  Database db;
  db.disableSource(sources);
  // refreshSources();
  for (int source : sources) emit sourceChanged(source);
}

void TextManager::addText() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedIndexes();
  if (indexes.isEmpty()) return;

  bool ok;
  QString text = QInputDialog::getMultiLineText(this, tr("Add Text:"),
                                                tr("Text:"), "", &ok);

  if (ok && !text.isEmpty()) {
    int row = indexes[0].row();
    const QModelIndex& f =
        reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
            ->index(row, 0);
    int source = f.data().toInt();
    Database db;
    db.addText(source, text, -1, false);

    // refreshSources();
    emit sourceChanged(source);

    ui->sourcesTable->selectRow(row);
    populateTexts(f);
  }
}

void TextManager::addSource() {
  bool ok;
  QString sourceName = QInputDialog::getText(
      this, tr("Add Source:"), tr("Source name:"), QLineEdit::Normal, "", &ok);

  if (ok && !sourceName.isEmpty()) {
    Database db;
    db.getSource(sourceName);
  }

  refreshSources();
  // emit sourcesChanged();
}

void TextManager::deleteSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();

  if (indexes.isEmpty()) return;

  QList<int> sources;
  for (QModelIndex index : indexes) {
    int row = index.row();
    const QModelIndex& f =
        reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
            ->index(row, 0);
    int source = f.data().toInt();
    sources << source;
  }
  Database db;
  db.deleteSource(sources);
  for (int source : sources) {
    if (source ==
        reinterpret_cast<TextModel*>(ui->textsTable->model())->getSource()) {
      reinterpret_cast<TextModel*>(ui->textsTable->model())->clear();
    }
    emit sourceDeleted(source);
  }

  // refreshSources();
  emit sourcesChanged();
}

void TextManager::changeSelectMethod(int i) {
  QSettings s;
  if (s.value("select_method").toInt() != i) s.setValue("select_method", i);
}

void TextManager::refreshSource(int source) {
  QList<QStandardItem*> items =
      reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
          ->findItems(QString::number(source));
  if (!items.isEmpty()) {
    int row = items[0]->row();
    QList<QStandardItem*> rowItems =
        reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
            ->takeRow(row);
    Database db;
    QVariantList newData = db.getSourceData(items[0]->text().toInt());

    rowItems[0]->setText(newData[0].toString());
    rowItems[1]->setText(newData[1].toString());
    rowItems[2]->setText(newData[2].toString());
    rowItems[3]->setText(newData[3].toString());
    if (newData[4].toDouble() == 0)
      rowItems[4]->setText("");
    else
      rowItems[4]->setText(QString::number(newData[4].toDouble(), 'f', 1));
    QString dis = (newData[5].isNull()) ? "Yes" : "";
    rowItems[5]->setText(dis);

    reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
        ->insertRow(row, rowItems);
  }
}

void TextManager::ui_deleteSource(int source) {
  QList<QStandardItem*> items =
      reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
          ->findItems(QString::number(source));
  if (!items.isEmpty()) {
    int row = items[0]->row();
    QList<QStandardItem*> rowItems =
        reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
            ->takeRow(row);
    for (QStandardItem* item : rowItems) delete item;
  }
}

void TextManager::refreshSources() {
  reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())->clear();

  QStringList headers;
  headers << "id"
          << "Source Name"
          << "# Items"
          << "Results"
          << "avg WPM"
          << "Disabled";
  reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
      ->setHorizontalHeaderLabels(headers);

  ui->sourcesTable->setColumnHidden(0, true);

  Database db;
  QList<QVariantList> rows = db.getSourcesData();
  for (const QVariantList& row : rows) {
    QList<QStandardItem*> items;
    items << new QStandardItem(row[0].toString());
    items << new QStandardItem(row[1].toString());
    items << new QStandardItem(row[2].toString());
    items << new QStandardItem(row[3].toString());
    if (row[4].toDouble() == 0)
      items << new QStandardItem("");
    else
      items << new QStandardItem(QString::number(row[4].toDouble(), 'f', 1));
    QString dis = (row[5].isNull()) ? "Yes" : "";
    items << new QStandardItem(dis);

    for (QStandardItem* item : items) item->setFlags(Qt::ItemIsEnabled);
    items[1]->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
        ->appendRow(items);
  }

  ui->sourcesTable->resizeColumnsToContents();
}

void TextManager::populateTexts(const QModelIndex& index) {
  const QModelIndex& f =
      reinterpret_cast<QStandardItemModel*>(ui->sourcesTable->model())
          ->index(index.row(), 0);
  emit sourceSelected(f.data().toInt());
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

  // if (lastText) delete lastText;
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
