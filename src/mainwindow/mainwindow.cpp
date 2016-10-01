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
      ui(new Ui::MainWindow),
      settingsWidget(new SettingsWidget),
      libraryWidget(new Library),
      performanceWidget(new PerformanceHistory) {
  ui->setupUi(this);

  this->populateProfiles();
  this->updateWindowTitle();

  QSettings s;

  // Actions
  connect(ui->action_Settings, &QAction::triggered, this->settingsWidget,
          &QWidget::show);
  connect(ui->action_Settings, &QAction::triggered, this->settingsWidget,
          &QWidget::activateWindow);
  connect(ui->action_Library, &QAction::triggered, this->libraryWidget,
          &QWidget::show);
  connect(ui->action_Library, &QAction::triggered, this->libraryWidget,
          &QWidget::activateWindow);
  connect(ui->action_Performance, &QAction::triggered, this->performanceWidget,
          &QWidget::show);
  connect(ui->action_Performance, &QAction::triggered, this->performanceWidget,
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
  connect(ui->quizzer, &Quizzer::newStatistics, ui->statisticsWidget,
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

  // Performance
  connect(this->performanceWidget, &PerformanceHistory::setText, ui->quizzer,
          &Quizzer::setText);
  connect(this->performanceWidget, &PerformanceHistory::gotoTab, this,
          &MainWindow::gotoTab);
  connect(this->performanceWidget, &PerformanceHistory::settingsChanged,
          ui->plot, &LivePlot::updatePlotTargetLine);

  // Analysis
  connect(ui->statisticsWidget, &StatisticsWidget::newItems,
          ui->lessonGenWidget, &LessonGenWidget::addItems);
  connect(ui->statisticsWidget, &StatisticsWidget::newItems, this,
          &MainWindow::gotoLessonGenTab);

  // Lesson Generator
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson, this->libraryWidget,
          &Library::refreshSources);
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson, this->libraryWidget,
          &QWidget::show);
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson, this->libraryWidget,
          &QWidget::activateWindow);

  // Training Generator
  connect(ui->trainingGenWidget, &TrainingGenWidget::newTraining,
          this->libraryWidget, &Library::refreshSources);
  connect(ui->trainingGenWidget, &TrainingGenWidget::newTraining,
          this->libraryWidget, &QWidget::show);
  connect(ui->trainingGenWidget, &TrainingGenWidget::newTraining,
          this->libraryWidget, &QWidget::activateWindow);

  // Settings
  connect(settingsWidget, &SettingsWidget::settingsChanged, ui->quizzer,
          &Quizzer::setTyperFont);
  connect(settingsWidget, &SettingsWidget::settingsChanged, ui->plot,
          &LivePlot::updatePlotTargetLine);
  connect(settingsWidget, &SettingsWidget::settingsChanged, ui->quizzer,
          &Quizzer::updateTyperDisplay);
  connect(settingsWidget, &SettingsWidget::settingsChanged,
          this->performanceWidget, &PerformanceHistory::refreshCurrentPlot);
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
  connect(this, &MainWindow::profileChanged, ui->statisticsWidget,
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
  delete this->libraryWidget;
  delete this->performanceWidget;
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

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
void MainWindow::gotoLessonGenTab() { ui->tabWidget->setCurrentIndex(3); }

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
