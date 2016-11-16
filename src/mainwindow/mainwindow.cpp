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

#include "mainwindow/mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDirIterator>
#include <QMessageBox>
#include <QSettings>
#include <QSize>
#include <QStandardPaths>

#include <QsLog.h>

#include "config.h"
#include "database/db.h"
#include "mainwindow/liveplot/liveplot.h"
#include "texts/library.h"
#include "texts/text.h"
#include "ui_mainwindow.h"

using std::make_unique;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(make_unique<Ui::MainWindow>()) {
  ui->setupUi(this);
  ui->menuView->addAction(ui->plotDock->toggleViewAction());
  ui->menuTest->addAction(ui->quizzer->restartAction());
  ui->menuTest->addAction(ui->quizzer->cancelAction());
  ui->menuView->addAction(ui->mapDock->toggleViewAction());

  // Actions
  connect(ui->actionQuit, &QAction::triggered, this, &QWidget::close);
  connect(ui->actionSettings, &QAction::triggered, &settings_, &QWidget::show);
  connect(ui->actionSettings, &QAction::triggered, &settings_,
          &QWidget::activateWindow);
  connect(ui->actionLibrary, &QAction::triggered, &library_, &QWidget::show);
  connect(ui->actionLibrary, &QAction::triggered, &library_,
          &QWidget::activateWindow);
  connect(ui->actionPerformance, &QAction::triggered, &performance_,
          &QWidget::show);
  connect(ui->actionPerformance, &QAction::triggered, &performance_,
          &QWidget::activateWindow);
  connect(ui->actionLesson_Generator, &QAction::triggered, &lesson_generator_,
          &QWidget::show);
  connect(ui->actionLesson_Generator, &QAction::triggered, &lesson_generator_,
          &QWidget::activateWindow);
  connect(ui->actionTraining_Generator, &QAction::triggered,
          &training_generator_, &QWidget::show);
  connect(ui->actionTraining_Generator, &QAction::triggered,
          &training_generator_, &QWidget::activateWindow);
  connect(ui->actionAnalysis, &QAction::triggered, &statistics_,
          &QWidget::show);
  connect(ui->actionAnalysis, &QAction::triggered, &statistics_,
          &QWidget::activateWindow);
  connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::aboutDialog);
  connect(ui->actionGrindWords, &QAction::triggered, ui->quizzer,
          &Quizzer::actionGrindWords);
  connect(ui->actionGrindViscWords, &QAction::triggered, ui->quizzer,
          &Quizzer::actionGrindViscWords);
  connect(ui->actionGrindInaccurateWords, &QAction::triggered, ui->quizzer,
          &Quizzer::actionGrindInaccurateWords);
  connect(ui->actionGrindDamagingWords, &QAction::triggered, ui->quizzer,
          &Quizzer::actionGrindDamagingWords);
  // Keyboard map
  connect(ui->mapComboBox, &QComboBox::currentTextChanged, ui->keyboardMap,
          &KeyboardMap::setData);
  // Typer
  connect(ui->quizzer, &Quizzer::newResult, &performance_,
          &PerformanceHistory::refresh);
  connect(ui->quizzer, &Quizzer::newResult, &library_, &Library::refreshSource);
  connect(ui->quizzer, &Quizzer::newStatistics, &statistics_,
          &StatisticsWidget::populateStatistics);
  connect(ui->quizzer, &Quizzer::newStatistics, ui->keyboardMap,
          &KeyboardMap::updateData);
  // Live Plot
  connect(ui->quizzer, &Quizzer::newWpm, ui->plot, &LivePlot::addWpm);
  connect(ui->quizzer, &Quizzer::testStarted, ui->plot, &LivePlot::beginTest);
  // Library
  connect(&library_, &Library::setText, ui->quizzer, &Quizzer::setText);
  connect(&library_, &Library::sourcesChanged, &performance_,
          &PerformanceHistory::refreshSources);
  connect(&library_, &Library::sourcesChanged, &performance_,
          &PerformanceHistory::refresh);
  connect(&library_, &Library::sourcesDeleted, ui->quizzer,
          &Quizzer::checkSource);
  connect(&library_, &Library::textsDeleted, ui->quizzer, &Quizzer::checkText);
  connect(&library_, &Library::textsChanged, ui->quizzer, &Quizzer::checkText);
  // Performance
  connect(&performance_, &PerformanceHistory::setText, ui->quizzer,
          &Quizzer::setText);
  connect(&performance_, &PerformanceHistory::settingsChanged, ui->plot,
          &LivePlot::updatePlotTargetLine);
  // Analysis
  connect(&statistics_, &StatisticsWidget::newItems, &lesson_generator_,
          &LessonGenWidget::addItems);
  connect(&statistics_, &StatisticsWidget::newItems, &lesson_generator_,
          &QWidget::show);
  connect(&statistics_, &StatisticsWidget::newItems, &lesson_generator_,
          &QWidget::activateWindow);
  // Lesson Generator
  connect(&lesson_generator_, &LessonGenWidget::newLesson, &library_,
          &Library::refreshSources);
  connect(&lesson_generator_, &LessonGenWidget::newLesson, &lesson_generator_,
          &QWidget::close);
  connect(&lesson_generator_, &LessonGenWidget::newLesson, &library_,
          &QWidget::show);
  connect(&lesson_generator_, &LessonGenWidget::newLesson, &library_,
          &QWidget::activateWindow);
  connect(&lesson_generator_, &LessonGenWidget::newLesson, &library_,
          &Library::selectSource);
  // Training Generator
  connect(&training_generator_, &TrainingGenWidget::newTraining, &library_,
          &Library::refreshSources);
  connect(&training_generator_, &TrainingGenWidget::newTraining, &library_,
          &QWidget::show);
  connect(&training_generator_, &TrainingGenWidget::newTraining, &library_,
          &QWidget::activateWindow);
  connect(&training_generator_, &TrainingGenWidget::newTraining,
          &training_generator_, &QWidget::close);
  connect(&training_generator_, &TrainingGenWidget::newTraining, &library_,
          &Library::selectSource);
  // Settings
  connect(&settings_, &SettingsWidget::settingsChanged, ui->quizzer,
          &Quizzer::loadSettings);
  connect(&settings_, &SettingsWidget::settingsChanged, ui->plot,
          &LivePlot::updatePlotTargetLine);
  connect(&settings_, &SettingsWidget::settingsChanged, &performance_,
          &PerformanceHistory::loadSettings);
  connect(&settings_, &SettingsWidget::newKeyboard, ui->keyboardMap,
          &KeyboardMap::setKeyboard);
  // Profile Changed
  connect(this, &MainWindow::profileChanged, ui->quizzer,
          &Quizzer::onProfileChange);
  connect(this, &MainWindow::profileChanged, &library_,
          &Library::onProfileChange);
  connect(this, &MainWindow::profileChanged, &performance_,
          &PerformanceHistory::onProfileChange);
  connect(this, &MainWindow::profileChanged, &statistics_,
          &StatisticsWidget::onProfileChange);
  connect(this, &MainWindow::profileChanged, this,
          &MainWindow::onProfileChange);
  connect(this, &MainWindow::profileChanged, ui->keyboardMap,
          &KeyboardMap::onProfileChange);

  loadSettings();
}

MainWindow::~MainWindow() {}

void MainWindow::loadSettings() {
  QSettings s;
  restoreState(s.value("mainWindow/windowState").toByteArray());
  restoreGeometry(s.value("mainWindow/windowGeometry").toByteArray());
  performance_.restoreState(
      s.value("performanceWindow/windowState").toByteArray());
  performance_.restoreGeometry(
      s.value("performanceWindow/windowGeometry").toByteArray());
  library_.restoreState(s.value("libraryWindow/windowState").toByteArray());
  library_.restoreGeometry(
      s.value("libraryWindow/windowGeometry").toByteArray());
  changeProfile(s.value("profile", "default").toString());
}

void MainWindow::saveSettings() {
  QSettings s;
  s.setValue("mainWindow/windowState", saveState());
  s.setValue("mainWindow/windowGeometry", saveGeometry());
  s.setValue("performanceWindow/windowState", performance_.saveState());
  s.setValue("performanceWindow/windowGeometry", performance_.saveGeometry());
  s.setValue("libraryWindow/windowState", library_.saveState());
  s.setValue("libraryWindow/windowGeometry", library_.saveGeometry());
  settings_.saveSettings();
  ui->quizzer->saveSettings();
  performance_.saveSettings();
  statistics_.saveSettings();
}

void MainWindow::onProfileChange() {
  updateWindowTitle();
  populateProfiles();
}

void MainWindow::populateProfiles() {
  ui->menuProfiles->clear();

  auto a_create = ui->menuProfiles->addAction(tr("New profile"));
  auto a_compress = ui->menuProfiles->addAction(tr("Compress database"));
  connect(a_create, &QAction::triggered, this, &MainWindow::createProfile);
  connect(a_compress, &QAction::triggered, this, [this] { db_->compress(); });

  ui->menuProfiles->addSeparator();

  QDirIterator it(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
  while (it.hasNext()) {
    it.next();
    if (it.fileName().endsWith(".profile")) {
      auto name = it.fileName().split(".profile")[0];
      auto a_profile = ui->menuProfiles->addAction(name);
      connect(a_profile, &QAction::triggered, this,
              [this, name] { changeProfile(name); });
    }
  }
}

void MainWindow::updateWindowTitle() {
  QSettings s;
  this->setWindowTitle(
      QString("amphetype2 - %1").arg(s.value("profile", "default").toString()));
}

void MainWindow::createProfile() {
  QLOG_DEBUG() << "new profile";
  bool ok;
  auto newName = QInputDialog::getText(this, tr("Create Profile"), tr("Name:"),
                                       QLineEdit::Normal, "", &ok);
  if (ok && !newName.isEmpty()) changeProfile(newName);
}
void MainWindow::changeProfile(const QString& name) {
  QLOG_DEBUG() << "changeProfile" << name;
  QSettings s;
  s.setValue("profile", name);
  db_.reset(new Database(name));
  db_->initDB();
  emit profileChanged(name);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  saveSettings();
  qApp->quit();
}

void MainWindow::aboutDialog() {
  QMessageBox::about(this, QCoreApplication::applicationName(),
                     QString("Version %1\nQt %2")
                         .arg(amphetype2_VERSION_STRING_FULL)
                         .arg(QT_VERSION_STR));
}
