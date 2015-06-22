#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "text.h"

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui(new Ui::MainWindow)
{
        ui->setupUi(this);

        connect(ui->quizzer,     SIGNAL(wantText(Text*)),
                ui->textManager, SLOT  (nextText(Text*)));

        connect(ui->textManager, SIGNAL(setText(Text*)),
                ui->quizzer,     SLOT  (setText(Text*)));
        connect(ui->textManager, SIGNAL(gotoTab(int)),
                this,            SLOT  (gotoTab(int)));

        connect(ui->performanceHistory, SIGNAL(setText(Text*)),
                ui->quizzer,            SLOT  (setText(Text*)));
        connect(ui->performanceHistory, SIGNAL(gotoTab(int)),
                this,                   SLOT  (gotoTab(int)));

        connect(ui->tabWidget,   SIGNAL(currentChanged(int)),
                ui->textManager, SLOT  (tabActive(int)));
        connect(ui->tabWidget,   SIGNAL(currentChanged(int)),
                ui->quizzer,     SLOT  (tabActive(int)));

        connect(ui->settingsWidget, SIGNAL(settingsChanged()),
                ui->quizzer,        SLOT  (setTyperFont()));

        ui->quizzer->wantText(0);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
