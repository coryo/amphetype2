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
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QString>
#include <QUrl>
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <QsLog.h>

#include "database/db.h"
#include "generators/lessongenwidget.h"
#include "texts/edittextdialog.h"
#include "texts/lessonminer.h"
#include "texts/lessonminercontroller.h"
#include "texts/text.h"
#include "ui_library.h"

Library::Library(QWidget* parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::Library>()) {
  ui->setupUi(this);
  ui->actionAdd_Text->setEnabled(false);
  ui->sourcesTable->setItemDelegate(new DatabaseItemDelegate(this));
  ui->textsTable->setItemDelegate(new DatabaseItemDelegate(this));

  connect(ui->actionImport_Texts, &QAction::triggered, this,
          &Library::addFiles);
  connect(ui->actionImport_XML, &QAction::triggered, this,
          &Library::importSource);
  connect(ui->actionAdd_Source, &QAction::triggered, this, &Library::addSource);
  connect(ui->actionAdd_Text, &QAction::triggered, this, &Library::addText);
  connect(ui->textsTable, &QTableView::doubleClicked, this,
          [this](const QModelIndex& index) {
            emit setText(db_->getText(index.data(Qt::UserRole).toInt()));
          });
  connect(ui->sourcesTable, &QWidget::customContextMenuRequested, this,
          &Library::sourcesContextMenu);
  connect(ui->textsTable, &QWidget::customContextMenuRequested, this,
          &Library::textsContextMenu);
  connect(ui->actionClose, &QAction::triggered, this, &QWidget::close);
}

Library::~Library() {}

void Library::sourceSelectionChanged(const QItemSelection& a,
                                     const QItemSelection& b) {
  if (!ui->sourcesTable->selectionModel()->hasSelection()) {
    ui->actionAdd_Text->setEnabled(false);
    db_text_model_->setPageSize(0);
    db_text_model_->clear();
  } else {
    ui->actionAdd_Text->setEnabled(true);
    if (a.indexes().isEmpty()) return;
    int source = a.indexes()[0].data(Qt::UserRole).toInt();
    db_text_model_->setPageSize(20);
    db_text_model_->setSource(source);
    db_text_model_->clear();
  }
}

void Library::onProfileChange() {
  db_.reset(new Database);

  db_source_model_.reset(new DatabaseModel("source"));
  db_source_model_->setHorizontalHeaderLabels(
      QStringList() << "id" << tr("Name") << tr("Texts") << tr("Results")
                    << tr("WPM") << "disabled"
                    << "type");
  db_source_model_->populate();
  ui->sourcesTable->setModel(db_source_model_.get());
  connect(ui->sourcesTable->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &Library::sourceSelectionChanged);
  ui->sourcesTable->setColumnHidden(0, true);
  ui->sourcesTable->setColumnHidden(5, true);
  ui->sourcesTable->setColumnHidden(6, true);

  db_text_model_.reset(new TextPagedDatabaseModel("text", -1, 0));
  db_text_model_->setHorizontalHeaderLabels(
      QStringList() << "id" << tr("Text") << tr("Length") << tr("Results")
                    << tr("WPM") << tr("Disabled") << "source");
  ui->textsTable->setModel(db_text_model_.get());
  ui->textsTable->setColumnHidden(0, true);
  ui->textsTable->setColumnHidden(6, true);

  auto sourceHeader = ui->sourcesTable->horizontalHeader();
  auto textHeader = ui->textsTable->horizontalHeader();
  sourceHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
  sourceHeader->setSectionResizeMode(1, QHeaderView::Stretch);
  textHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
  textHeader->setSectionResizeMode(1, QHeaderView::Stretch);
}

void Library::sourcesContextMenu(const QPoint& pos) {
  auto selected = ui->sourcesTable->selectionModel()->selectedRows();
  QList<int> sources;
  for (const auto& idx : selected) sources << idx.data(Qt::UserRole).toInt();

  QMenu menu(this);
  QAction* a_delete = menu.addAction(tr("Delete"));
  QAction* a_enable = menu.addAction(tr("Enable all texts"));
  QAction* a_disable = menu.addAction(tr("Disable all texts"));
  QAction* a_export = menu.addAction(tr("Export as XML..."));

  connect(a_delete, &QAction::triggered, this, [this, sources, selected] {
    QMessageBox msgBox(QMessageBox::Warning, tr("Confirm"),
                       tr("Confirm delete source(s)."),
                       QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setInformativeText(
        tr("Deleting a source will delete all associated results."));
    if (msgBox.exec() == QMessageBox::Cancel) return;
    db_source_model_->removeIndexes(selected);
    db_text_model_->setPageSize(0);
    db_text_model_->clear();
    emit sourcesDeleted(sources);
    emit sourcesChanged();
  });
  connect(a_enable, &QAction::triggered, this, [this, sources] {
    db_->enableSource(sources);
    db_text_model_->clear();
  });
  connect(a_disable, &QAction::triggered, this, [this, sources] {
    db_->disableSource(sources);
    db_text_model_->clear();
  });
  connect(a_export, &QAction::triggered, this, &Library::exportSource);

  menu.exec(QCursor::pos());
}

void Library::textsContextMenu(const QPoint& pos) {
  auto selected = ui->textsTable->selectionModel()->selectedRows();
  QList<int> texts;
  for (const auto& row : selected) texts << row.data(Qt::UserRole).toInt();

  QMenu menu(this);
  QAction* a_enable = menu.addAction(tr("Enable"));
  QAction* a_disable = menu.addAction(tr("Disable"));
  QAction* a_delete = menu.addAction(tr("Delete"));

  if (texts.count() == 1) {
    QAction* a_send = menu.addAction(tr("Send to Typer"));
    QAction* a_edit = menu.addAction(tr("Edit"));
    const int id = texts[0];
    const auto idx = ui->textsTable->selectionModel()->selectedRows(1)[0];
    connect(a_send, &QAction::triggered, this,
            [this, id] { emit setText(db_->getText(id)); });
    connect(a_edit, &QAction::triggered, this,
            [this, idx] { ui->textsTable->edit(idx); });
  }

  connect(a_enable, &QAction::triggered, this, [this, selected, texts] {
    db_->enableText(texts);
    db_text_model_->refreshIndexes(selected);
  });
  connect(a_disable, &QAction::triggered, this, [this, selected, texts] {
    db_->disableText(texts);
    db_text_model_->refreshIndexes(selected);
  });
  connect(a_delete, &QAction::triggered, this, [this, selected, texts] {
    db_text_model_->removeIndexes(selected);
    if (ui->sourcesTable->selectionModel()->hasSelection()) {
      auto source = ui->sourcesTable->selectionModel()->selectedRows()[0];
      db_source_model_->refreshIndex(source);
      ui->sourcesTable->update(source);
      emit sourceChanged(source.data(Qt::UserRole).toInt());
    }
    emit textsDeleted(texts);
  });

  menu.exec(QCursor::pos());
}

void Library::exportSource() {
  if (!ui->sourcesTable->selectionModel()->hasSelection()) return;
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export XML"), ".",
                                                  tr("XML files (*.xml)"));
  QLOG_DEBUG() << fileName;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) return;

  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  stream.writeStartDocument();
  stream.writeStartElement("sources");
  for (const auto& index : indexes) {
    int source = index.data(Qt::UserRole).toInt();
    auto sourceData = db_->getSourceData(source);
    stream.writeStartElement("source");
    stream.writeAttribute("name", sourceData[1].toString());
    if (sourceData[6].toInt() == 1) stream.writeAttribute("type", "lesson");
    QStringList texts = db_->getAllTexts(source);
    for (const QString& text : texts) {
      stream.writeTextElement("text", text);
    }
    stream.writeEndElement();
  }
  stream.writeEndElement();
  stream.writeEndDocument();

  this->validateXml(&file);
}

void Library::importSource() {
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Import"), ".", tr("amphetype2 source xml (*.xml)"));
  QFile file(fileName);

  if (!this->validateXml(&file)) return;
  file.open(QIODevice::ReadOnly | QIODevice::Text);

  QXmlStreamReader xml(&file);
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.name() == "source") {
      if (xml.attributes().value("name").isEmpty()) continue;

      QStringList texts;
      auto type = amphetype::text_type::Standard;

      if (xml.attributes().value("type") == "lesson") {
        type = amphetype::text_type::Lesson;
      }

      int source =
          db_->getSource(xml.attributes().value("name").toString(), type);
      while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) {
          if (!texts.isEmpty()) db_->addTexts(source, texts);
          break;
        }
        if (xml.name() == "text") {
          QString text(xml.readElementText());
          if (!text.isEmpty()) texts << text;
        }
      }
    }
  }
  if (xml.hasError()) return;

  db_source_model_->rowAdded();
  emit sourcesChanged();
}

void Library::addText() {
  if (!ui->sourcesTable->selectionModel()->hasSelection()) return;
  auto indexes = ui->sourcesTable->selectionModel()->selectedRows();

  bool ok;
  QString text = QInputDialog::getMultiLineText(this, tr("Add Text:"),
                                                tr("Text:"), "", &ok);
  if (ok && !text.isEmpty()) {
    db_->addText(indexes[0].data(Qt::UserRole).toInt(), text);
    db_source_model_->refreshIndex(indexes[0]);
    db_text_model_->rowAdded();
  }
}

void Library::addSource() {
  bool ok;
  auto name =
      QInputDialog::getText(this, tr("Add Source"), tr("Source name") + ":",
                            QLineEdit::Normal, "", &ok);
  if (ok && !name.isEmpty()) {
    db_->getSource(name);
    db_source_model_->rowAdded();
    emit sourcesChanged();
  }
}

void Library::refreshSources() { db_source_model_->populate(); }

void Library::refreshSource(int source) {
  db_source_model_->refreshItem(qMakePair<QString, QVariant>("id", source));
}

void Library::selectSource(int source) {
  for (int i = 0; i < db_source_model_->rowCount(); i++) {
    auto index = db_source_model_->index(i, 0);
    if (index.data(Qt::UserRole) == source)
      return ui->sourcesTable->setCurrentIndex(index);
  }
}

void Library::addFiles() {
  auto files = QFileDialog::getOpenFileNames(
      this, tr("Import"), ".", tr("UTF-8 text files (*.txt);;All files (*)"));
  if (files.isEmpty()) return;

  auto lmc = new LessonMinerController(files);
  auto progress = new QProgressDialog("Importing...", "Abort", 0, 100);
  progress->setMinimumDuration(0);
  progress->setAutoClose(false);

  connect(lmc, &LessonMinerController::progressUpdate, progress,
          &QProgressDialog::setValue);
  connect(lmc, &LessonMinerController::done, this, [this] {
    db_source_model_->populate();
    emit sourcesChanged();
  });
  connect(lmc, &LessonMinerController::done, lmc,
          &LessonMinerController::deleteLater);
  connect(lmc, &LessonMinerController::done, progress,
          &QProgressDialog::deleteLater);

  lmc->start();
}

bool Library::validateXml(QFile* file) {
  if (file->isOpen()) file->close();

  if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
    QLOG_DEBUG() << "validateXml: can't open file:" << file->fileName();
    return false;
  }

  QXmlSchema schema;
  QFile schemaFile(":/sources.xsd");
  schemaFile.open(QIODevice::ReadOnly);
  schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));
  bool v = false;
  if (schema.isValid()) {
    QXmlSchemaValidator validator(schema);
    v = validator.validate(file, QUrl::fromLocalFile(file->fileName()));
    QLOG_DEBUG() << "validateXml: document is" << (v ? "valid." : "invalid.")
                 << file->fileName();
  } else {
    QLOG_DEBUG() << "validateXml: schema is invalid." << schemaFile.fileName();
  }
  file->close();
  return v;
}
