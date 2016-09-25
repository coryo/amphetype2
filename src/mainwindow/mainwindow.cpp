#include "mainwindow/mainwindow.h"

#include <QSettings>

#include "ui_mainwindow.h"
#include "texts/text.h"
#include "quizzer/typer.h"
#include "liveplot/liveplot.h"
#include "texts/textmanager.h"


MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  QSettings s;

  connect(ui->quizzer,            &Quizzer::wantText,
          ui->textManager,        &TextManager::nextText);
  connect(ui->quizzer,            &Quizzer::newResult,
          ui->performanceHistory, &PerformanceHistory::refreshPerformance);
  connect(ui->quizzer,            &Quizzer::newStatistics,
          ui->statisticsWidget,   &StatisticsWidget::populateStatistics);

  connect(ui->actionPlot, &QAction::triggered, this, &MainWindow::togglePlot);
  ui->actionPlot->setChecked(s.value("liveplot_visible").toBool());
  // plot
  connect(ui->quizzer, &Quizzer::newWpm, ui->plot, &LivePlot::addWpm);
  connect(ui->quizzer, &Quizzer::newApm, ui->plot, &LivePlot::addApm);

  connect(ui->quizzer, &Quizzer::characterAdded,
          ui->plot,    &LivePlot::newKeyPress);
  connect(ui->quizzer, &Quizzer::testStarted,
          ui->plot,    &LivePlot::beginTest);
  connect(ui->quizzer,                            &Quizzer::newStatistics,
          ui->statisticsWidget->getKeyboardMap(), &KeyboardMap::addKeys);

  if (!s.value("liveplot_visible").toBool())
    ui->plotDock->close();

  connect(ui->textManager, &TextManager::setText,
          ui->quizzer, &Quizzer::setText);
  connect(ui->textManager, &TextManager::gotoTab, this, &MainWindow::gotoTab);
  connect(ui->textManager, &TextManager::sourceDeleted,
          ui->performanceHistory, &PerformanceHistory::refreshPerformance);

  connect(ui->performanceHistory, &PerformanceHistory::setText,
          ui->quizzer,            &Quizzer::setText);
  connect(ui->performanceHistory, &PerformanceHistory::gotoTab,
          this,                   &MainWindow::gotoTab);
  connect(ui->performanceHistory, &PerformanceHistory::settingsChanged,
          ui->plot,               &LivePlot::updatePlotTargetLine);

  connect(ui->tabWidget,   &QTabWidget::currentChanged,
          ui->textManager, &TextManager::tabActive);
  connect(ui->tabWidget,   &QTabWidget::currentChanged,
          ui->quizzer,     &Quizzer::tabActive);

  connect(ui->statisticsWidget, &StatisticsWidget::newItems,
          ui->lessonGenWidget,  &LessonGenWidget::addItems);
  connect(ui->statisticsWidget, &StatisticsWidget::newItems,
          this,                 &MainWindow::gotoLessonGenTab);

  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson,
          this,                &MainWindow::gotoSourcesTab);
  connect(ui->lessonGenWidget, &LessonGenWidget::newLesson,
          ui->textManager,     &TextManager::refreshSources);


  connect(ui->settingsWidget, &SettingsWidget::settingsChanged,
          ui->quizzer,        &Quizzer::setTyperFont);
  connect(ui->settingsWidget, &SettingsWidget::settingsChanged,
          ui->plot,           &LivePlot::updatePlotTargetLine);
  connect(ui->settingsWidget, &SettingsWidget::settingsChanged,
          ui->quizzer,        &Quizzer::updateTyperDisplay);
  connect(ui->settingsWidget,     &SettingsWidget::settingsChanged,
          ui->performanceHistory, &PerformanceHistory::refreshCurrentPlot);

  // auto map = ui->statisticsWidget->getKeyboardMap()
  connect(ui->settingsWidget, &SettingsWidget::newKeyboard,
          ui->statisticsWidget->getKeyboardMap(), &KeyboardMap::setKeyboard);

  connect(ui->trainingGenWidget, &TrainingGenWidget::generatedLessons,
          ui->textManager,       &TextManager::refreshSources);
  connect(ui->trainingGenWidget, &TrainingGenWidget::generatedLessons,
          this,                  &MainWindow::gotoSourcesTab);

  ui->quizzer->wantText(0, SelectionMethod::Random);
}

void MainWindow::togglePlot(bool checked) {
  QSettings s;
  s.setValue("liveplot_visible", checked);
  ui->plotDock->setVisible(checked);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
void MainWindow::gotoTyperTab() { ui->tabWidget->setCurrentIndex(0); }
void MainWindow::gotoSourcesTab() { ui->tabWidget->setCurrentIndex(1); }
void MainWindow::gotoLessonGenTab() { ui->tabWidget->setCurrentIndex(4); }
