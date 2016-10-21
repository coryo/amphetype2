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

#include "settings/settingswidget.h"

#include <QDir>
#include <QFont>
#include <QFontDialog>
#include <QSettings>

#include <QsLog.h>

#include "ui_settingswidget.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingsWidget) {
  ui->setupUi(this);

  this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

  loadSettings();

  connect(ui->fontButton, SIGNAL(pressed()), this, SLOT(selectFont()));
  connect(ui->styleSheetComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeStyleSheet(int)));

  connect(ui->disablePerformanceLoggingCheckBox, SIGNAL(stateChanged(int)),
          this, SLOT(saveSettings()));

  connect(ui->targetWPMSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->targetAccSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(saveSettings()));
  connect(ui->targetVisSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(saveSettings()));
  connect(ui->debugLoggingCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->selectionMethod, SIGNAL(currentIndexChanged(int)), this,
          SLOT(saveSettings()));

  connect(ui->keyboardLayoutComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeKeyboardLayout(int)));
  connect(ui->keyboardStandardComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeKeyboardStandard(int)));
  connect(ui->keyboardLayoutComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(saveSettings()));
  connect(ui->keyboardStandardComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(saveSettings()));

  connect(ui->closeButton, &QPushButton::pressed, this, &QWidget::close);
}

SettingsWidget::~SettingsWidget() {
  delete ui;
}

void SettingsWidget::loadSettings() {
  QSettings s;
  ui->selectionMethod->setCurrentIndex(s.value("select_method", 0).toInt());
  ui->styleSheetComboBox->addItem("Dark Theme", "dark-1");
  ui->styleSheetComboBox->addItem("Basic Theme", "basic");
  ui->styleSheetComboBox->addItem("QDarkstyle", "qdarkstyle");
  ui->styleSheetComboBox->setCurrentIndex(ui->styleSheetComboBox->findData(
      s.value("stylesheet", "basic").toString()));

  ui->disablePerformanceLoggingCheckBox->setCheckState(
      s.value("perf_logging", true).toBool() ? Qt::Checked : Qt::Unchecked);

  ui->targetWPMSpinBox->setValue(s.value("target_wpm", 50).toInt());
  ui->targetAccSpinBox->setValue(s.value("target_acc", 97).toDouble());
  ui->targetVisSpinBox->setValue(s.value("target_vis", 2).toDouble());

  ui->keyboardStandardComboBox->setCurrentIndex(
      s.value("keyboard_standard", 0).toInt());
  ui->keyboardLayoutComboBox->setCurrentIndex(
      s.value("keyboard_layout", 0).toInt());

  ui->debugLoggingCheckBox->setCheckState(
      s.value("debug_logging", false).toBool() ? Qt::Checked : Qt::Unchecked);
}

void SettingsWidget::saveSettings() {
  QSettings s;
  s.setValue("keyboard_layout", ui->keyboardLayoutComboBox->currentIndex());
  s.setValue("keyboard_standard", ui->keyboardStandardComboBox->currentIndex());
  s.setValue("select_method", ui->selectionMethod->currentIndex());
  s.setValue("target_wpm", ui->targetWPMSpinBox->value());
  s.setValue("target_acc", ui->targetAccSpinBox->value());
  s.setValue("target_vis", ui->targetVisSpinBox->value());
  s.setValue(
      "perf_logging",
      ui->disablePerformanceLoggingCheckBox->checkState() == Qt::Checked);

  // Debugging
  s.setValue("debug_logging",
             ui->debugLoggingCheckBox->checkState() == Qt::Checked);
  if (ui->debugLoggingCheckBox->checkState() == Qt::Checked)
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::DebugLevel);
  else
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::InfoLevel);

  emit settingsChanged();
}

void SettingsWidget::changeStyleSheet(int i) {
  QSettings s;
  QString ss = ui->styleSheetComboBox->itemData(i).toString();

  QFile file(":/stylesheets/" + ss + ".qss");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qApp->setStyleSheet(file.readAll());
    file.close();
    s.setValue("stylesheet", ss);
    emit settingsChanged();
  }
}

void SettingsWidget::selectFont() {
  QSettings s;
  bool ok;
  QFont font =
      QFontDialog::getFont(&ok, qvariant_cast<QFont>(s.value("Quizzer/typer_font")));
  if (ok) {
    s.setValue("Quizzer/typer_font", font);
    emit settingsChanged();
  }
}

void SettingsWidget::changeKeyboardLayout(int index) {
  emit newKeyboard(static_cast<amphetype::Layout>(index),
                   static_cast<amphetype::Standard>(
                       ui->keyboardStandardComboBox->currentIndex()));
}

void SettingsWidget::changeKeyboardStandard(int index) {
  emit newKeyboard(static_cast<amphetype::Layout>(
                       ui->keyboardLayoutComboBox->currentIndex()),
                   static_cast<amphetype::Standard>(index));
}
