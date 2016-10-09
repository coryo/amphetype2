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
#include <QDirIterator>
#include <QSettings>
#include <QSize>

#include <QsLog.h>

#include "database/db.h"
#include "mainwindow/liveplot/liveplot.h"
#include "quizzer/typer.h"
#include "texts/library.h"
#include "texts/text.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      settingsWidget(new SettingsWidget),
      statisticsWidget(new StatisticsWidget),
      libraryWidget(new Library),
      performanceWidget(new PerformanceHistory),
      lessonGenerator(new LessonGenWidget),
      trainingGenerator(new TrainingGenWidget),
      ui(new Ui::MainWindow) {
  ui->setupUi(this);

  this->populateProfiles();
  this->updateWindowTitle();

  QSettings s;

  // Actions
  connect(ui->actionQuit, &QAction::triggered, this, &QWidget::close);
  connect(ui->actionSettings, &QAction::triggered, this->settingsWidget,
          &QWidget::show);
  connect(ui->actionSettings, &QAction::triggered, this->settingsWidget,
          &QWidget::activateWindow);
  connect(ui->actionLibrary, &QAction::triggered, this->libraryWidget,
          &QWidget::show);
  connect(ui->actionLibrary, &QAction::triggered, this->libraryWidget,
          &QWidget::activateWindow);
  connect(ui->actionPerformance, &QAction::triggered, this->performanceWidget,
          &QWidget::show);
  connect(ui->actionPerformance, &QAction::triggered, this->performanceWidget,
          &QWidget::activateWindow);
  connect(ui->actionLesson_Generator, &QAction::triggered,
          this->lessonGenerator, &QWidget::show);
  connect(ui->actionLesson_Generator, &QAction::triggered,
          this->lessonGenerator, &QWidget::activateWindow);
  connect(ui->actionTraining_Generator, &QAction::triggered,
          this->trainingGenerator, &QWidget::show);
  connect(ui->actionTraining_Generator, &QAction::triggered,
          this->trainingGenerator, &QWidget::activateWindow);
  connect(ui->actionAnalysis, &QAction::triggered, this->statisticsWidget,
          &QWidget::show);
  connect(ui->actionAnalysis, &QAction::triggered, this->statisticsWidget,
          &QWidget::activateWindow);

  ui->menuView->addAction(ui->plotDock->toggleViewAction());

  connect(ui->mapComboBox, &QComboBox::currentTextChanged, ui->keyboardMap,
          &KeyboardMap::setData);
  ui->keyboardMap->updateData();
  ui->menuView->addAction(ui->mapDock->toggleViewAction());

  // Typer
  connect(ui->quizzer, &Quizzer::wantText, this->libraryWidget,
          &Library::nextText);
  connect(ui->quizzer, &Quizzer::newResult, this->performanceWidget,
          &PerformanceHistory::refreshPerformance);
  connect(ui->quizzer, &Quizzer::newResult, this->libraryWidget,
          &Library::refreshSource);
  connect(ui->quizzer, &Quizzer::newStatistics, this->statisticsWidget,
          &StatisticsWidget::update);
  connect(ui->quizzer, &Quizzer::newStatistics, ui->keyboardMap,
          &KeyboardMap::updateData);
  // Live Plot
  connect(ui->quizzer, &Quizzer::newWpm, ui->plot, &LivePlot::addWpm);
  connect(ui->quizzer, &Quizzer::newApm, ui->plot, &LivePlot::addApm);
  connect(ui->quizzer, &Quizzer::characterAdded, ui->plot,
          &LivePlot::newKeyPress);
  connect(ui->quizzer, &Quizzer::testStarted, ui->plot, &LivePlot::beginTest);

  // Library
  connect(this->libraryWidget, &Library::setText, ui->quizzer,
          &Quizzer::setText);
  connect(this->libraryWidget, &Library::sourcesChanged,
          this->performanceWidget, &PerformanceHistory::refreshSources);
  connect(this->libraryWidget, &Library::sourcesChanged,
          this->performanceWidget, &PerformanceHistory::refreshPerformance);
  connect(this->libraryWidget, &Library::sourcesDeleted, ui->quizzer,
          &Quizzer::checkSource);
  connect(this->libraryWidget, &Library::textsDeleted, ui->quizzer,
          &Quizzer::checkText);
  connect(this->libraryWidget, &Library::textsChanged, ui->quizzer,
          &Quizzer::checkText);

  // Performance
  connect(this->performanceWidget, &PerformanceHistory::setText, ui->quizzer,
          &Quizzer::setText);
  // connect(this->performanceWidget, &PerformanceHistory::gotoTab, this,
  //         &MainWindow::gotoTab);
  connect(this->performanceWidget, &PerformanceHistory::settingsChanged,
          ui->plot, &LivePlot::updatePlotTargetLine);

  // Analysis
  connect(this->statisticsWidget, &StatisticsWidget::newItems,
          this->lessonGenerator, &LessonGenWidget::addItems);
  connect(this->statisticsWidget, &StatisticsWidget::newItems,
          this->lessonGenerator, &QWidget::show);
  connect(this->statisticsWidget, &StatisticsWidget::newItems,
          this->lessonGenerator, &QWidget::activateWindow);

  // Lesson Generator
  connect(this->lessonGenerator, &LessonGenWidget::newLesson,
          this->libraryWidget, &Library::refreshSources);
  connect(this->lessonGenerator, &LessonGenWidget::newLesson,
          this->lessonGenerator, &QWidget::close);
  connect(this->lessonGenerator, &LessonGenWidget::newLesson,
          this->libraryWidget, &QWidget::show);
  connect(this->lessonGenerator, &LessonGenWidget::newLesson,
          this->libraryWidget, &QWidget::activateWindow);
  connect(this->lessonGenerator, &LessonGenWidget::newLesson,
          this->libraryWidget, &Library::selectSource);

  // Training Generator
  connect(this->trainingGenerator, &TrainingGenWidget::newTraining,
          this->libraryWidget, &Library::refreshSources);
  connect(this->trainingGenerator, &TrainingGenWidget::newTraining,
          this->libraryWidget, &QWidget::show);
  connect(this->trainingGenerator, &TrainingGenWidget::newTraining,
          this->libraryWidget, &QWidget::activateWindow);
  connect(this->trainingGenerator, &TrainingGenWidget::newTraining,
          this->trainingGenerator, &QWidget::close);
  connect(this->trainingGenerator, &TrainingGenWidget::newTraining,
          this->libraryWidget, &Library::selectSource);

  // Settings
  connect(settingsWidget, &SettingsWidget::settingsChanged, ui->quizzer,
          &Quizzer::loadSettings);
  connect(settingsWidget, &SettingsWidget::settingsChanged, ui->plot,
          &LivePlot::updatePlotTargetLine);
  connect(settingsWidget, &SettingsWidget::settingsChanged,
          this->performanceWidget, &PerformanceHistory::loadSettings);
  connect(settingsWidget, &SettingsWidget::newKeyboard, ui->keyboardMap,
          &KeyboardMap::setKeyboard);

  connect(ui->menuProfiles, &QMenu::triggered, this,
          &MainWindow::changeProfile);

  // Profile Changed
  connect(this, &MainWindow::profileChanged, this->libraryWidget,
          &Library::reload);
  connect(this, &MainWindow::profileChanged, this->performanceWidget,
          &PerformanceHistory::refreshPerformance);
  connect(this, &MainWindow::profileChanged, this->performanceWidget,
          &PerformanceHistory::refreshSources);
  connect(this, &MainWindow::profileChanged, this->statisticsWidget,
          &StatisticsWidget::update);
  connect(this, &MainWindow::profileChanged, ui->keyboardMap,
          &KeyboardMap::updateData);
  connect(this, &MainWindow::profileChanged, this,
          &MainWindow::updateWindowTitle);
  connect(this, &MainWindow::profileChanged, this,
          &MainWindow::populateProfiles);

  this->restoreState(s.value("mainWindow/windowState").toByteArray());
  this->restoreGeometry(s.value("mainWindow/windowGeometry").toByteArray());
  performanceWidget->restoreState(
      s.value("performanceWindow/windowState").toByteArray());
  performanceWidget->restoreGeometry(
      s.value("performanceWindow/windowGeometry").toByteArray());
  libraryWidget->restoreState(
      s.value("libraryWindow/windowState").toByteArray());
  libraryWidget->restoreGeometry(
      s.value("libraryWindow/windowGeometry").toByteArray());

  ui->quizzer->wantText(0, static_cast<Amphetype::SelectionMethod>(
                               s.value("select_method", 0).toInt()));
}

MainWindow::~MainWindow() {
  delete ui;
  delete this->settingsWidget;
  delete this->statisticsWidget;
  delete this->libraryWidget;
  delete this->performanceWidget;
  delete this->lessonGenerator;
  delete this->trainingGenerator;
  Database db;
  db.compress();
}

void MainWindow::populateProfiles() {
  ui->menuProfiles->clear();
  QAction* create = new QAction("Create New Profile", this);
  create->setData(true);
  ui->menuProfiles->addAction(create);
  ui->menuProfiles->addSeparator();
  QDirIterator it(qApp->applicationDirPath());
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

  ui->quizzer->wantText(0, static_cast<Amphetype::SelectionMethod>(
                               s.value("select_method", 0).toInt()));
}

void MainWindow::closeEvent(QCloseEvent* event) {
  QSettings s;
  QSize size = this->size();
  s.setValue("mainWindow/windowState", this->saveState());
  s.setValue("mainWindow/windowGeometry", this->saveGeometry());
  s.setValue("performanceWindow/windowState", performanceWidget->saveState());
  s.setValue("performanceWindow/windowGeometry",
             performanceWidget->saveGeometry());
  s.setValue("libraryWindow/windowState", libraryWidget->saveState());
  s.setValue("libraryWindow/windowGeometry", libraryWidget->saveGeometry());
  qApp->quit();
}
