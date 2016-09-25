#include "texts/textmanager.h"

#include <QString>
#include <QFileDialog>
#include <QSettings>
#include <QFile>
#include <QProgressBar>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QInputDialog>

#include <QsLog.h>

#include "ui_textmanager.h"
#include "texts/lessonminer.h"
#include "texts/lessonminercontroller.h"
#include "database/db.h"
#include "texts/text.h"

TextManager::TextManager(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::TextManager),
  refreshed(false),
  sourcesModel(new QStandardItemModel),
  textsModel(new QStandardItemModel) {
  ui->setupUi(this);

  QSettings s;

  ui->sourcesTable->setModel(sourcesModel);
  ui->sourcesTable->verticalHeader()->setVisible(false);
  ui->textsTable->setModel(textsModel);
  ui->textsTable->verticalHeader()->setVisible(false);
  ui->textsWidget->hide();

  // progress bar/text setup
  ui->progress->setRange(0, 100);
  ui->progress->hide();
  ui->progressText->hide();

  ui->selectionMethod->setCurrentIndex(s.value("select_method").toInt());

  connect(ui->importButton,        SIGNAL(clicked()),
          this,                    SLOT(addFiles()));
  connect(ui->refreshSources,      SIGNAL(clicked()),
          this,                    SLOT(refreshSources()));
  connect(ui->addSourceButton,     SIGNAL(clicked()),
          this,                    SLOT(addSource()));
  connect(ui->deleteSourceButton,  SIGNAL(clicked()),
          this,                    SLOT(deleteSource()));
  connect(ui->addTextButton,       SIGNAL(clicked()),
          this,                    SLOT(addText()));
  connect(ui->deleteTextButton,    SIGNAL(clicked()),
          this,                    SLOT(deleteText()));
  connect(ui->textsTable,          SIGNAL(doubleClicked(const QModelIndex&)),
          this,                    SLOT(doubleClicked(const QModelIndex&)));
  connect(ui->sourcesTable,        SIGNAL(pressed(const QModelIndex&)),
          this,                    SLOT(populateTexts(const QModelIndex&)));
  connect(ui->sourcesTable,        SIGNAL(doubleClicked(const QModelIndex&)),
          this,                    SLOT(toggleTextsWidget()));
  connect(ui->selectionMethod,     SIGNAL(currentIndexChanged(int)),
          this,                    SLOT(changeSelectMethod(int)));
  connect(ui->enableSourceButton,  SIGNAL(pressed()),
          this,                    SLOT(enableSource()));
  connect(ui->disableSourceButton, SIGNAL(pressed()),
          this,                    SLOT(disableSource()));
  connect(ui->showTextsButton,     SIGNAL(pressed()),
          this,                    SLOT(toggleTextsWidget()));
  connect(ui->editTextButton,      SIGNAL(pressed()),
          this,                    SLOT(editText()));
}

TextManager::~TextManager() {
  delete ui;
  delete sourcesModel;
  delete textsModel;
}

void TextManager::editText() {
  auto sourceIndexes = ui->sourcesTable->selectionModel()->selectedIndexes();
  auto textIndexes = ui->textsTable->selectionModel()->selectedIndexes();
  if (sourceIndexes.isEmpty() || textIndexes.isEmpty())
    return;

  auto textRow = textsModel->index(textIndexes[0].row(), 0);
  auto sourceRow = sourcesModel->index(sourceIndexes[0].row(), 0);

  int source = sourceRow.data().toInt();
  int rowid  = textRow.data().toInt();

  Text* t = DB::getText(rowid);

  bool ok;
  QString text = QInputDialog::getMultiLineText(this, tr("Edit Text:"),
    tr("Text:"), t->getText(), &ok);

  if (ok && !text.isEmpty()) {
    if (text == t->getText()) {
      delete t;
      return;
    }
    DB::updateText(rowid, text);
    refreshSources();
    ui->sourcesTable->selectRow(sourceRow.row());
    populateTexts(sourceRow);
  }
  delete t;
}

void TextManager::toggleTextsWidget() {
  if (ui->textsWidget->isVisible()) {
    ui->textsWidget->hide();
    ui->emptyWidget->show();
    textsModel->clear();
    ui->showTextsButton->setText("Show Texts ->");
  } else {
    ui->textsWidget->show();
    ui->emptyWidget->hide();
    ui->showTextsButton->setText("<- Hide Texts");
    auto indexes = ui->sourcesTable->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
      populateTexts(indexes[0]);
  }
}

void TextManager::enableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedIndexes();
  if (indexes.isEmpty())
    return;

  QList<int> sources;
  for (QModelIndex index : indexes) {
    int row = index.row();
    const QModelIndex& f = sourcesModel->index(row, 0);
    sources << f.data().toInt();
  }
  DB::enableSource(sources);
  refreshSources();
}

void TextManager::disableSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedIndexes();
  if (indexes.isEmpty())
    return;

  QList<int> sources;
  for (QModelIndex index : indexes) {
    int row = index.row();
    const QModelIndex& f = sourcesModel->index(row, 0);
    sources << f.data().toInt();
  }
  DB::disableSource(sources);
  refreshSources();
}

void TextManager::addText() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedIndexes();
  if (indexes.isEmpty())
    return;

  bool ok;
  QString text = QInputDialog::getMultiLineText(this, tr("Add Text:"),
    tr("Text:"), "", &ok);

  if (ok && !text.isEmpty()) {
    int row = indexes[0].row();
    const QModelIndex& f = sourcesModel->index(row, 0);
    int source = f.data().toInt();

    DB::addText(source, text, -1, false);

    refreshSources();
    ui->sourcesTable->selectRow(row);
    populateTexts(f);
  }
}

void TextManager::addSource() {
  bool ok;
  QString sourceName = QInputDialog::getText(this, tr("Add Source:"),
    tr("Source name:"), QLineEdit::Normal, "", &ok);

  if (ok && !sourceName.isEmpty())
    DB::getSource(sourceName);

  refreshSources();
}

void TextManager::deleteText() {
  auto sourceIndexes = ui->sourcesTable->selectionModel()->selectedIndexes();
  auto textIndexes = ui->textsTable->selectionModel()->selectedIndexes();
  if (textIndexes.isEmpty() || sourceIndexes.isEmpty())
    return;

  int sourceRow = sourceIndexes[0].row();
  const QModelIndex& source = sourcesModel->index(sourceRow, 0);

  QList<int> rowids;
  for (QModelIndex index : textIndexes) {
    int row = index.row();
    const QModelIndex& f = textsModel->index(row, 0);
    int rowid = f.data().toInt();
    rowids << rowid;
  }
  DB::deleteText(rowids);

  refreshSources();
  ui->sourcesTable->selectRow(sourceRow);
  populateTexts(source);
}

void TextManager::deleteSource() {
  auto indexes = ui->sourcesTable->selectionModel()->selectedIndexes();

  if (indexes.isEmpty())
    return;

  QList<int> sources;
  for (QModelIndex index : indexes) {
    int row = index.row();
    const QModelIndex& f = sourcesModel->index(row, 0);
    int source = f.data().toInt();
    sources << source;
  }
  DB::deleteSource(sources);

  refreshSources();
  ui->textsWidget->hide();
  emit sourceDeleted();
}

void TextManager::changeSelectMethod(int i) {
  QSettings s;
  if (s.value("select_method").toInt() != i)
    s.setValue("select_method", i);
}

void TextManager::tabActive(int i) {
  if ((i == 1) && (!refreshed)) {
    // do stuff when this tab is made active & the tree hasnt been loaded
    refreshSources();
  }
}

void TextManager::refreshSources() {
  ui->sourcesTable->hide();

  sourcesModel->clear();

  QStringList headers;
  headers << "id"
          << "Source Name"
          << "# Items"
          << "Results"
          << "avg WPM"
          << "Disabled";
  sourcesModel->setHorizontalHeaderLabels(headers);

  ui->sourcesTable->setSortingEnabled(false);
  ui->sourcesTable->setColumnHidden(0, true);
  ui->sourcesTable->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
  ui->sourcesTable->verticalHeader()->setDefaultSectionSize(24);

  refreshed = true;

  QList<QStringList> rows = DB::getSourcesData();
  for (QStringList row : rows) {
    QList<QStandardItem*> items;
    items << new QStandardItem(row[0]);
    items << new QStandardItem(row[1]);
    items << new QStandardItem(row[2]);
    items << new QStandardItem(row[3]);
    if (row[4].toDouble() == 0)
      items << new QStandardItem("");
    else
      items << new QStandardItem(
        QString::number(row[4].toDouble(), 'f', 1));
    QString dis = (row[5].isEmpty()) ? "Yes" : "";
    items << new QStandardItem(dis);

    for (QStandardItem* item : items)
      item->setFlags(Qt::ItemIsEnabled);
    items[1]->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    sourcesModel->appendRow(items);
  }

  ui->sourcesTable->setModel(sourcesModel);
  ui->sourcesTable->horizontalHeader()->setSectionResizeMode(
    1, QHeaderView::Stretch);
  ui->sourcesTable->resizeColumnsToContents();
  ui->sourcesTable->show();
}

void TextManager::populateTexts(const QModelIndex& index) {
  if (!ui->textsWidget->isVisible())
    return;
  ui->textsTable->hide();

  const QModelIndex& f = sourcesModel->index(index.row(), 0);

  textsModel->clear();

  QStringList headers;
  headers << "id"
          << "Text"
          << "Length"
          << "Results"
          << "med WPM"
          << "Dis";
  textsModel->setHorizontalHeaderLabels(headers);

  ui->textsTable->setSortingEnabled(false);
  ui->textsTable->setColumnHidden(0, true);
  ui->textsTable->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
  ui->textsTable->verticalHeader()->setDefaultSectionSize(24);

  QList<QStringList> rows = DB::getTextsData(f.data().toInt());
  for (QStringList row : rows) {
    QList<QStandardItem*> items;
    items << new QStandardItem(row[0]);
    items << new QStandardItem(row[1].simplified());
    items << new QStandardItem(row[2]);
    items << new QStandardItem(row[3]);
    if (row[4].toDouble() == 0)
      items << new QStandardItem("");
    else
      items << new QStandardItem(
        QString::number(row[4].toDouble(), 'f', 1));
    QString dis = (row[5].isEmpty()) ? "" : "Yes";
    items << new QStandardItem(dis);

    for (QStandardItem* item : items)
      item->setFlags(Qt::ItemIsEnabled);
    items[1]->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    textsModel->appendRow(items);
  }

  ui->textsTable->setModel(textsModel);
  ui->textsTable->resizeColumnsToContents();
  ui->textsTable->show();
}

void TextManager::doubleClicked(const QModelIndex& idx) {
  const QModelIndex& f = textsModel->index(idx.row(), 0);
  int rowid = f.data().toInt();

  Text* t = DB::getText(rowid);
  emit setText(t);
  emit gotoTab(0);
}

void TextManager::nextText(Text* lastText, SelectionMethod method) {
  QSettings s;
  SelectionMethod selectMethod;
  if (method != SelectionMethod::None) {
    selectMethod = static_cast<SelectionMethod>(method);
  } else {
    if (lastText && lastText->getType() > 0)
      selectMethod = SelectionMethod::InOrder;
    else
      selectMethod = static_cast<SelectionMethod>(
        s.value("select_method").toInt());
  }

  QLOG_DEBUG() << "TextManager::nextText" << "select_method ="
               << static_cast<int>(selectMethod);

  Text* nextText;

  switch (selectMethod) {
  case SelectionMethod::Random:
  case SelectionMethod::Ask:
    QLOG_DEBUG() << "nextText: Random";
    nextText = DB::getRandomText();
    emit setText(nextText);
    break;
  case SelectionMethod::InOrder:
    QLOG_DEBUG() << "nextText: In Order";
    if (!s.value("perf_logging").toBool() && lastText) {
      nextText = DB::getNextText(lastText);
      emit setText(nextText);
    } else {
      nextText = DB::getNextText();
      emit setText(nextText);
    }
    break;
  case SelectionMethod::Repeat:
    QLOG_DEBUG() << "nextText: Repeat";
    emit setText(lastText);
    break;
  }

  if (lastText)
    delete lastText;
}

void TextManager::addFiles() {
  files = QFileDialog::getOpenFileNames(
      this, tr("Import"), ".",
      tr("UTF-8 text files (*.txt);;All files (*)"));
  if (files.isEmpty())
    return;

  ui->progress->show();
  ui->progressText->show();
  lmc = new LessonMinerController;
  // lesson miner signals
  connect(lmc,  SIGNAL(workDone()),
          this, SLOT(processNextFile()));
  connect(lmc,          &LessonMinerController::progressUpdate,
          ui->progress, &QProgressBar::setValue);

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