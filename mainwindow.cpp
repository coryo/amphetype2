#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "typer.h"
#include "db.h"
#include "text.h"

#include <QtSql>
#include <QMessageBox>
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui(new Ui::MainWindow)
{
        QSettings s;

        if (!QSqlDatabase::drivers().contains("QSQLITE"))
                QMessageBox::critical(this, "Unable to load database",
                                      "This demo needs the SQLITE driver");

        // init
        QSqlError err = DB::initDb(s.value("db_name").toString());
        if (err.type() != QSqlError::NoError) {
                showError(err);
                return;
        }

        ui->setupUi(this);

        connect(this, SIGNAL(initText()), ui->textManager, SLOT(nextText()));

        connect(ui->quizzer, SIGNAL(wantText()), ui->textManager, SLOT(nextText()));

        connect(ui->textManager, SIGNAL(setText(Text*)), ui->quizzer, SLOT(setText(Text*)));
        connect(ui->textManager, SIGNAL(gotoTab(int)), this, SLOT(gotoTab(int)));

        connect(ui->performanceHistory, SIGNAL(setText(Text*)), ui->quizzer, SLOT(setText(Text*)));
        connect(ui->performanceHistory, SIGNAL(gotoTab(int)), this, SLOT(gotoTab(int)));

        connect(ui->tabWidget, SIGNAL(currentChanged(int)), ui->textManager, SLOT(tabActive(int)));
        connect(ui->tabWidget, SIGNAL(currentChanged(int)), ui->quizzer, SLOT(tabActive(int)));

        connect(ui->settingsWidget, SIGNAL(settingsChanged()), ui->quizzer, SLOT(setTyperFont()));

        emit initText();
}

MainWindow::~MainWindow()
{
        delete ui;
}

void MainWindow::showError(const QSqlError& err)
{
        QMessageBox::critical(this, "Unable to initialize Database",
                              "Error initializing database: " + err.text());
}

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
