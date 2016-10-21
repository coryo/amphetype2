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

#include <sqlite3.h>

#include "config.h"
#include "database/db.h"
#include "mainwindow/liveplot/liveplot.h"
#include "quizzer/typer.h"
#include "texts/library.h"
#include "texts/text.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      settings_(std::make_unique<SettingsWidget>()),
      statistics_(std::make_unique<StatisticsWidget>()),
      library_(std::make_unique<Library>()),
      performance_(std::make_unique<PerformanceHistory>()),
      lesson_generator_(std::make_unique<LessonGenWidget>()),
      training_generator_(std::make_unique<TrainingGenWidget>()),
      ui(new Ui::MainWindow) {
  ui->setupUi(this);

  populateProfiles();
  updateWindowTitle();

  // Actions
  connect(ui->actionQuit, &QAction::triggered, this, &QWidget::close);
  connect(ui->actionSettings, &QAction::triggered, settings_.get(),
          &QWidget::show);
  connect(ui->actionSettings, &QAction::triggered, settings_.get(),
          &QWidget::activateWindow);
  connect(ui->actionLibrary, &QAction::triggered, library_.get(),
          &QWidget::show);
  connect(ui->actionLibrary, &QAction::triggered, library_.get(),
          &QWidget::activateWindow);
  connect(ui->actionPerformance, &QAction::triggered, performance_.get(),
          &QWidget::show);
  connect(ui->actionPerformance, &QAction::triggered, performance_.get(),
          &QWidget::activateWindow);
  connect(ui->actionLesson_Generator, &QAction::triggered,
          lesson_generator_.get(), &QWidget::show);
  connect(ui->actionLesson_Generator, &QAction::triggered,
          lesson_generator_.get(), &QWidget::activateWindow);
  connect(ui->actionTraining_Generator, &QAction::triggered,
          training_generator_.get(), &QWidget::show);
  connect(ui->actionTraining_Generator, &QAction::triggered,
          training_generator_.get(), &QWidget::activateWindow);
  connect(ui->actionAnalysis, &QAction::triggered, statistics_.get(),
          &QWidget::show);
  connect(ui->actionAnalysis, &QAction::triggered, statistics_.get(),
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

  ui->menuView->addAction(ui->plotDock->toggleViewAction());

  connect(ui->mapComboBox, &QComboBox::currentTextChanged, ui->keyboardMap,
          &KeyboardMap::setData);
  ui->keyboardMap->updateData();
  ui->menuView->addAction(ui->mapDock->toggleViewAction());

  // Typer
  connect(ui->quizzer->typer(), &Typer::newResult, performance_.get(),
          &PerformanceHistory::refreshPerformance);
  connect(ui->quizzer->typer(), &Typer::newResult, library_.get(),
          &Library::refreshSource);
  connect(ui->quizzer->typer(), &Typer::newStatistics, statistics_.get(),
          &StatisticsWidget::update);
  connect(ui->quizzer->typer(), &Typer::newStatistics, ui->keyboardMap,
          &KeyboardMap::updateData);
  // Live Plot
  connect(ui->quizzer->typer(), &Typer::newWpm, ui->plot, &LivePlot::addWpm);
  connect(ui->quizzer->typer(), &Typer::testStarted, ui->plot,
          &LivePlot::beginTest);

  // Library
  connect(library_.get(), &Library::setText, ui->quizzer, &Quizzer::setText);
  connect(library_.get(), &Library::sourcesChanged, performance_.get(),
          &PerformanceHistory::refreshSources);
  connect(library_.get(), &Library::sourcesChanged, performance_.get(),
          &PerformanceHistory::refreshPerformance);
  connect(library_.get(), &Library::sourcesDeleted, ui->quizzer,
          &Quizzer::checkSource);
  connect(library_.get(), &Library::textsDeleted, ui->quizzer,
          &Quizzer::checkText);
  connect(library_.get(), &Library::textsChanged, ui->quizzer,
          &Quizzer::checkText);

  // Performance
  connect(performance_.get(), &PerformanceHistory::setText, ui->quizzer,
          &Quizzer::setText);
  connect(performance_.get(), &PerformanceHistory::settingsChanged, ui->plot,
          &LivePlot::updatePlotTargetLine);

  // Analysis
  connect(statistics_.get(), &StatisticsWidget::newItems,
          lesson_generator_.get(), &LessonGenWidget::addItems);
  connect(statistics_.get(), &StatisticsWidget::newItems,
          lesson_generator_.get(), &QWidget::show);
  connect(statistics_.get(), &StatisticsWidget::newItems,
          lesson_generator_.get(), &QWidget::activateWindow);

  // Lesson Generator
  connect(lesson_generator_.get(), &LessonGenWidget::newLesson, library_.get(),
          &Library::refreshSources);
  connect(lesson_generator_.get(), &LessonGenWidget::newLesson,
          lesson_generator_.get(), &QWidget::close);
  connect(lesson_generator_.get(), &LessonGenWidget::newLesson, library_.get(),
          &QWidget::show);
  connect(lesson_generator_.get(), &LessonGenWidget::newLesson, library_.get(),
          &QWidget::activateWindow);
  connect(lesson_generator_.get(), &LessonGenWidget::newLesson, library_.get(),
          &Library::selectSource);

  // Training Generator
  connect(training_generator_.get(), &TrainingGenWidget::newTraining,
          library_.get(), &Library::refreshSources);
  connect(training_generator_.get(), &TrainingGenWidget::newTraining,
          library_.get(), &QWidget::show);
  connect(training_generator_.get(), &TrainingGenWidget::newTraining,
          library_.get(), &QWidget::activateWindow);
  connect(training_generator_.get(), &TrainingGenWidget::newTraining,
          training_generator_.get(), &QWidget::close);
  connect(training_generator_.get(), &TrainingGenWidget::newTraining,
          library_.get(), &Library::selectSource);

  // Settings
  connect(settings_.get(), &SettingsWidget::settingsChanged, ui->quizzer,
          &Quizzer::loadSettings);
  connect(settings_.get(), &SettingsWidget::settingsChanged, ui->plot,
          &LivePlot::updatePlotTargetLine);
  connect(settings_.get(), &SettingsWidget::settingsChanged, performance_.get(),
          &PerformanceHistory::loadSettings);
  connect(settings_.get(), &SettingsWidget::newKeyboard, ui->keyboardMap,
          &KeyboardMap::setKeyboard);

  connect(ui->menuProfiles, &QMenu::triggered, this,
          &MainWindow::changeProfile);

  // Profile Changed
  connect(this, &MainWindow::profileChanged, library_.get(), &Library::reload);
  connect(this, &MainWindow::profileChanged, performance_.get(),
          &PerformanceHistory::refreshPerformance);
  connect(this, &MainWindow::profileChanged, performance_.get(),
          &PerformanceHistory::refreshSources);
  connect(this, &MainWindow::profileChanged, statistics_.get(),
          &StatisticsWidget::update);
  connect(this, &MainWindow::profileChanged, ui->keyboardMap,
          &KeyboardMap::updateData);
  connect(this, &MainWindow::profileChanged, this,
          &MainWindow::updateWindowTitle);
  connect(this, &MainWindow::profileChanged, this,
          &MainWindow::populateProfiles);

  QSettings s;
  restoreState(s.value("mainWindow/windowState").toByteArray());
  restoreGeometry(s.value("mainWindow/windowGeometry").toByteArray());
  performance_->restoreState(
      s.value("performanceWindow/windowState").toByteArray());
  performance_->restoreGeometry(
      s.value("performanceWindow/windowGeometry").toByteArray());
  library_->restoreState(s.value("libraryWindow/windowState").toByteArray());
  library_->restoreGeometry(
      s.value("libraryWindow/windowGeometry").toByteArray());

  ui->quizzer->setText(Text::selectText(static_cast<amphetype::SelectionMethod>(
      s.value("select_method", 0).toInt())));
}

MainWindow::~MainWindow() {
  delete ui;
  Database db;
  db.compress();
}

void MainWindow::populateProfiles() {
  ui->menuProfiles->clear();
  QAction* create = new QAction("Create New Profile", this);
  create->setData(true);
  ui->menuProfiles->addAction(create);
  ui->menuProfiles->addSeparator();
  QDirIterator it(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
  while (it.hasNext()) {
    it.next();
    QString file(it.fileName());
    if (file.endsWith(".profile")) {
      QAction* profile = new QAction(file.split(".profile")[0], this);
      ui->menuProfiles->addAction(profile);
    }
  }
}

void MainWindow::updateWindowTitle() {
  QSettings s;
  this->setWindowTitle(
      QString("amphetype2 - %1").arg(s.value("profile", "default").toString()));
}

void MainWindow::changeProfile(QAction* action) {
  QString name;
  if (!action->data().isNull()) {
    QLOG_DEBUG() << "new profile";
    bool ok;
    QString newName = QInputDialog::getText(
        this, tr("Create Profile"), tr("Name:"), QLineEdit::Normal, "", &ok);

    if (ok && !newName.isEmpty()) {
      name = newName;
    } else {
      return;
    }
  } else {
    name = action->text();
  }

  QLOG_DEBUG() << "changeProfile" << name;
  QSettings s;
  if (s.value("profile", "default").toString() == name) return;
  s.setValue("profile", name);
  Database db(name);
  db.initDB();
  emit profileChanged(name);
  ui->quizzer->setText(Text::selectText(static_cast<amphetype::SelectionMethod>(
      s.value("select_method", 0).toInt())));
}

void MainWindow::closeEvent(QCloseEvent* event) {
  QSettings s;
  QSize size = this->size();
  s.setValue("mainWindow/windowState", saveState());
  s.setValue("mainWindow/windowGeometry", saveGeometry());
  s.setValue("performanceWindow/windowState", performance_->saveState());
  s.setValue("performanceWindow/windowGeometry", performance_->saveGeometry());
  s.setValue("libraryWindow/windowState", library_->saveState());
  s.setValue("libraryWindow/windowGeometry", library_->saveGeometry());
  settings_->saveSettings();
  ui->quizzer->saveSettings();
  performance_->saveSettings();
  statistics_->saveSettings();
  qApp->quit();
}

void MainWindow::aboutDialog() {
  QMessageBox::about(this, QCoreApplication::applicationName(),
                     QString("Version %1\nQt %2\nSQLite %3")
                         .arg(amphetype2_VERSION_STRING_FULL)
                         .arg(QT_VERSION_STR)
                         .arg(SQLITE_VERSION));
}
