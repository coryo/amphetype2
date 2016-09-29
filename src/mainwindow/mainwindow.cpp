#include "mainwindow/mainwindow.h"

#include <QApplication>
#include <QSettings>
#include <QSize>

#include "liveplot/liveplot.h"
#include "quizzer/typer.h"
#include "texts/text.h"
#include "texts/textmanager.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      settingsWidget(new SettingsWidget),
      libraryWidget(new TextManager) {
  ui->setupUi(this);

  QSettings s;

  // Set UI state
  this->settingsWidget->hide();
  this->libraryWidget->hide();

  // Actions
  connect(ui->action_Settings, &QAction::triggered, this->settingsWidget,
          &QWidget::show);
  connect(ui->action_Settings, &QAction::triggered, this->settingsWidget,
          &QWidget::activateWindow);
  connect(ui->action_Library, &QAction::triggered, this->libraryWidget,
          &QWidget::show);
  connect(ui->action_Library, &QAction::triggered, this->libraryWidget,
          &QWidget::activateWindow);

  ui->menuView->addAction(ui->plotDock->toggleViewAction());

  connect(ui->mapComboBox, &QComboBox::currentTextChanged, ui->keyboardMap,
          &KeyboardMap::setData);
  ui->keyboardMap->updateData();
  ui->menuView->addAction(ui->mapDock->toggleViewAction());

  // Typer
  connect(ui->quizzer, &Quizzer::wantText, this->libraryWidget,
          &TextManager::nextText);
  connect(ui->quizzer, &Quizzer::newResult, ui->performanceHistory,
          &PerformanceHistory::refreshPerformance);
  connect(ui->quizzer, &Quizzer::newResult, this->libraryWidget,
          &TextManager::refreshSource);
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
  connect(this->libraryWidget, &TextManager::setText, ui->quizzer,
          &Quizzer::setText);
  connect(this->libraryWidget, &TextManager::sourcesChanged,
          ui->performanceHistory, &PerformanceHistory::refreshPerformance);

  // Performance
  connect(ui->performanceHistory, &PerformanceHistory::setText, ui->quizzer,
          &Quizzer::setText);
  connect(ui->performanceHistory, &PerformanceHistory::gotoTab, this,
          &MainWindow::gotoTab);
  connect(ui->performanceHistory, &PerformanceHistory::settingsChanged,
          ui->plot, &LivePlot::updatePlotTargetLine);

  // Analysis
  connect(ui->statisticsWidget, &StatisticsWidget::newItems,
          ui->lessonGenWidget, &LessonGenWidget::addItems);
  connect(ui->statisticsWidget, &StatisticsWidget::newItems, this,
          &MainWindow::gotoLessonGenTab);

  // Lesson Generator
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson, this->libraryWidget,
          &TextManager::refreshSources);
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson, this->libraryWidget,
          &QWidget::show);
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson, this->libraryWidget,
          &QWidget::activateWindow);

  // Training Generator
  connect(ui->trainingGenWidget, &TrainingGenWidget::newTraining,
          this->libraryWidget, &TextManager::refreshSources);
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
          ui->performanceHistory, &PerformanceHistory::refreshCurrentPlot);
  connect(settingsWidget, &SettingsWidget::newKeyboard, ui->keyboardMap,
          &KeyboardMap::setKeyboard);

  this->restoreState(s.value("mainWindow/windowState").toByteArray());
  this->restoreGeometry(s.value("mainWindow/windowGeometry").toByteArray());
  ui->quizzer->wantText(0, Amphetype::SelectionMethod::Random);
}

MainWindow::~MainWindow() {
  delete ui;
  delete this->settingsWidget;
  delete this->libraryWidget;
}

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
void MainWindow::gotoLessonGenTab() { ui->tabWidget->setCurrentIndex(3); }

void MainWindow::closeEvent(QCloseEvent* event) {
  QSettings s;
  QSize size = this->size();
  s.setValue("mainWindow/windowState", this->saveState());
  s.setValue("mainWindow/windowGeometry", this->saveGeometry());
  qApp->quit();
}
