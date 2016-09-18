#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "text.h"
#include "typer.h"
#include "liveplot.h"

#include <QSettings>

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui(new Ui::MainWindow)
{
        ui->setupUi(this);

        QSettings s;

        connect(ui->quizzer,            &Quizzer::wantText,
                ui->textManager,        &TextManager::nextText);
        connect(ui->quizzer,            &Quizzer::newResult,
                ui->performanceHistory, &PerformanceHistory::refreshPerformance);
        connect(ui->quizzer,            &Quizzer::newStatistics,
                ui->statisticsWidget,   &StatisticsWidget::populateStatistics);

        connect(ui->actionPlot, &QAction::triggered, this, &MainWindow::togglePlot);//ui->plotDock, &QWidget::show);
        ui->actionPlot->setChecked(s.value("liveplot_visible").toBool());
        // plot
        connect(ui->quizzer, &Quizzer::newPoint,       ui->plot, &LivePlot::addPlotPoint);
        connect(ui->quizzer, &Quizzer::characterAdded, ui->plot, &LivePlot::newKeyPress);
        connect(ui->quizzer, &Quizzer::testStarted,    ui->plot, &LivePlot::beginTest);

        if (!s.value("liveplot_visible").toBool()) {
                ui->plotDock->close();
        }


        connect(ui->textManager, SIGNAL(setText(Text*)),
                ui->quizzer,     SLOT  (setText(Text*)));
        connect(ui->textManager, SIGNAL(gotoTab(int)),
                this,            SLOT  (gotoTab(int)));

        connect(ui->performanceHistory, SIGNAL(setText(Text*)),
                ui->quizzer,            SLOT  (setText(Text*)));
        connect(ui->performanceHistory, SIGNAL(gotoTab(int)),
                this,                   SLOT  (gotoTab(int)));
        connect(ui->performanceHistory, SIGNAL(settingsChanged()),
                ui->plot,               SLOT  (updatePlotTargetLine()));

        connect(ui->tabWidget,   SIGNAL(currentChanged(int)),
                ui->textManager, SLOT  (tabActive(int)));
        connect(ui->tabWidget,   SIGNAL(currentChanged(int)),
                ui->quizzer,     SLOT  (tabActive(int)));

        connect(ui->settingsWidget, SIGNAL(settingsChanged()),
                ui->quizzer,        SLOT  (setTyperFont()));
        connect(ui->settingsWidget, SIGNAL(settingsChanged()),
                ui->plot,        SLOT(updatePlotTargetLine()));
        connect(ui->settingsWidget, SIGNAL(settingsChanged()),
                ui->quizzer,        SLOT(updateTyperDisplay()));
        connect(ui->settingsWidget, SIGNAL(settingsChanged()),
                ui->performanceHistory, SLOT(refreshCurrentPlot()));

        connect(ui->trainingGenWidget, SIGNAL(generatedLessons()),
                ui->textManager,       SLOT(refreshSources()));
        connect(ui->trainingGenWidget, SIGNAL(generatedLessons()),
                this,                  SLOT(gotoSourcesTab()));

        ui->quizzer->wantText(0);
}

void MainWindow::togglePlot(bool checked)
{
        QSettings s;
        s.setValue("liveplot_visible", checked);
        ui->plotDock->setVisible(checked);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
void MainWindow::gotoTyperTab()   { ui->tabWidget->setCurrentIndex(0); }
void MainWindow::gotoSourcesTab() { ui->tabWidget->setCurrentIndex(1); }
