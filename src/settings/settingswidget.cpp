#include "settings/settingswidget.h"

#include <QFontDialog>
#include <QFont>
#include <QDir>
#include <QSettings>

#include <QsLog.h>

#include "ui_settingswidget.h"


SettingsWidget::SettingsWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsWidget) {
  ui->setupUi(this);

  QSettings s;

  ui->fontLabel->setFont(qvariant_cast<QFont>(s.value("typer_font")));

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

  bool debugLogging = s.value("debug_logging").toBool();
  ui->debugLoggingCheckBox->setCheckState(
    debugLogging ? Qt::Checked : Qt::Unchecked);

  connect(ui->fontButton, SIGNAL(pressed()),
          this,           SLOT(selectFont()));
  connect(ui->styleSheetComboBox, SIGNAL(currentIndexChanged(int)),
          this,                   SLOT(changeStyleSheet(int)));
  connect(ui->disablePerformanceLoggingCheckBox, SIGNAL(stateChanged(int)),
          this,                                  SLOT(changePerfLogging(int)));

  connect(ui->targetWPMSpinBox, SIGNAL(valueChanged(int)),
          this,                 SLOT(writeTargets()));
  connect(ui->targetAccSpinBox, SIGNAL(valueChanged(double)),
          this,                 SLOT(writeTargets()));
  connect(ui->targetVisSpinBox, SIGNAL(valueChanged(double)),
          this,                 SLOT(writeTargets()));

  connect(ui->debugLoggingCheckBox, SIGNAL(stateChanged(int)),
          this,                     SLOT(changeDebugLogging(int)));
}

SettingsWidget::~SettingsWidget() {
  delete ui;
}

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

  QFile file(":/stylesheets/"+ss+".qss");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qApp->setStyleSheet(file.readAll());
    file.close();
    s.setValue("stylesheet", ss);
  }
}

void SettingsWidget::selectFont() {
  QSettings s;
  bool ok;
  QFont font = QFontDialog::getFont(
    &ok, qvariant_cast<QFont>(s.value("typer_font")));

  if (ok) {
    s.setValue("typer_font", font);
    ui->fontLabel->setFont(font);
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
