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

#include <QFontDialog>
#include <QFont>
#include <QDir>
#include <QSettings>

#include <QsLog.h>

#include "ui_settingswidget.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingsWidget) {
  ui->setupUi(this);

  this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

  QSettings s;

  ui->selectionMethod->setCurrentIndex(s.value("select_method").toInt());

  ui->styleSheetComboBox->addItem("Dark Theme", "dark-1");
  ui->styleSheetComboBox->addItem("Basic Theme", "basic");
  ui->styleSheetComboBox->setCurrentIndex(
      ui->styleSheetComboBox->findData(s.value("stylesheet").toString()));

  bool perfLogging = s.value("perf_logging").toBool();
  ui->disablePerformanceLoggingCheckBox->setCheckState(
      perfLogging ? Qt::Checked : Qt::Unchecked);

  ui->targetWPMSpinBox->setValue(s.value("target_wpm").toInt());
  ui->targetAccSpinBox->setValue(s.value("target_acc").toDouble());
  ui->targetVisSpinBox->setValue(s.value("target_vis").toDouble());

  ui->keyboardStandardComboBox->setCurrentIndex(
      s.value("keyboard_standard", 0).toInt());
  ui->keyboardLayoutComboBox->setCurrentIndex(
      s.value("keyboard_layout", 0).toInt());

  bool debugLogging = s.value("debug_logging").toBool();
  ui->debugLoggingCheckBox->setCheckState(debugLogging ? Qt::Checked
                                                       : Qt::Unchecked);

  connect(ui->fontButton, SIGNAL(pressed()), this, SLOT(selectFont()));
  connect(ui->styleSheetComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeStyleSheet(int)));
  connect(ui->disablePerformanceLoggingCheckBox, SIGNAL(stateChanged(int)),
          this, SLOT(changePerfLogging(int)));

  connect(ui->targetWPMSpinBox, SIGNAL(valueChanged(int)), this,
          SLOT(writeTargets()));
  connect(ui->targetAccSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(writeTargets()));
  connect(ui->targetVisSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(writeTargets()));

  connect(ui->debugLoggingCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(changeDebugLogging(int)));

  connect(ui->keyboardLayoutComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeKeyboardLayout(int)));
  connect(ui->keyboardStandardComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeKeyboardStandard(int)));

  connect(ui->closeButton, &QPushButton::pressed, this, &QWidget::close);

  connect(ui->selectionMethod, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeSelectMethod(int)));
}

SettingsWidget::~SettingsWidget() { delete ui; }

void SettingsWidget::writeTargets() {
  QSettings s;
  s.setValue("target_wpm", ui->targetWPMSpinBox->value());
  s.setValue("target_acc", ui->targetAccSpinBox->value());
  s.setValue("target_vis", ui->targetVisSpinBox->value());
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
  }
}

void SettingsWidget::selectFont() {
  QSettings s;
  bool ok;
  QFont font =
      QFontDialog::getFont(&ok, qvariant_cast<QFont>(s.value("typer_font")));

  if (ok) {
    s.setValue("typer_font", font);
    // ui->fontLabel->setFont(font);
    emit settingsChanged();
  }
}

void SettingsWidget::changePerfLogging(int state) {
  QSettings s;
  s.setValue("perf_logging", state);
}

void SettingsWidget::changeDebugLogging(int state) {
  QSettings s;
  s.setValue("debug_logging", state);
  if (!state) {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::InfoLevel);
    QLOG_INFO() << "Debug logging disabled.";
  } else {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::DebugLevel);
    QLOG_INFO() << "Debug logging enabled.";
  }
}

void SettingsWidget::changeKeyboardLayout(int index) {
  QSettings s;
  s.setValue("keyboard_layout", index);
  emit newKeyboard(
      static_cast<Amphetype::Layout>(index),
      static_cast<Amphetype::Standard>(s.value("keyboard_standard", 0).toInt()));
}

void SettingsWidget::changeKeyboardStandard(int index) {
  QSettings s;
  s.setValue("keyboard_standard", index);
  emit newKeyboard(
      static_cast<Amphetype::Layout>(s.value("keyboard_layout", 0).toInt()),
      static_cast<Amphetype::Standard>(index));
}

void SettingsWidget::changeSelectMethod(int i) {
  QSettings s;
  if (s.value("select_method").toInt() != i) s.setValue("select_method", i);
}