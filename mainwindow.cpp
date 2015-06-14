#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "typer.h"
#include "db.h"
#include "text.h"

#include <QtSql>
#include <QMessageBox>
#include <iostream>



MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui(new Ui::MainWindow),
          s(new QSettings(qApp->applicationDirPath() + QDir::separator() +
                                  "Amphetype2.ini",
                          QSettings::IniFormat))
{
        loadSettings();

        if (!QSqlDatabase::drivers().contains("QSQLITE"))
                QMessageBox::critical(this, "Unable to load database",
                                      "This demo needs the SQLITE driver");

        // init
        QSqlError err = DB::initDb(s->value("db_name").toString());
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

        emit initText();
}

MainWindow::~MainWindow()
{
        delete ui;
        delete s;
}

void MainWindow::showError(const QSqlError& err)
{
        QMessageBox::critical(this, "Unable to initialize Database",
                              "Error initializing database: " + err.text());
}

void MainWindow::loadSettings()
{
        if (s->contains("typer_font")) {
                // std::cout<< "settings found" << std::endl;
        } else {
                // so settings found, write defaults
                s->setValue("typer_font", QFont("Consolas", 12));
                s->setValue("history", 30.0);
                s->setValue("min_chars", 220);
                s->setValue("max_chars", 600);
                s->setValue("lesson_stats", 0);
                s->setValue("perf_group_by", 0);
                s->setValue("perf_items", 100);
                s->setValue("text_regex", "");
                s->setValue("db_name", "db.sqlite");
                s->setValue("select_method", 0);
                s->setValue("num_rand", 50);
                s->setValue("graph_what", 3);
                s->setValue("req_space", false);
                s->setValue("show_last", true);
                s->setValue("show_xaxis", false);
                s->setValue("chrono_x", false);
                s->setValue("dampen_graph", false);
                s->setValue("minutes_in_sitting", 60.0);
                s->setValue("dampen_average", 10);
                s->setValue("def_group_by", 10);
                s->setValue("use_lesson_stats", false);
                s->setValue("auto_review", false);
                s->setValue("min_wpm", 0.0);
                s->setValue("min_acc", 0.0);
                s->setValue("min_lesson_wpm", 0.0);
                s->setValue("min_lesson_acc", 97.0);
                s->setValue("quiz_right_fg", "#000000");
                s->setValue("quiz_right_bg", "#ffffff");
                s->setValue("quiz_wrong_fg", "#ffffff");
                s->setValue("quiz_wrong_bg", "#000000");
                s->setValue("group_month", 365.0);
                s->setValue("group_week", 30.0);
                s->setValue("group_day", 7.0);
                s->setValue("ana_which", "wpm asc");
                s->setValue("ana_what", 0);
                s->setValue("ana_many", 30);
                s->setValue("ana_count", 1);
                s->setValue("gen_copies", 3);
                s->setValue("gen_take", 2);
                s->setValue("gen_mix", 'c');
                s->setValue("str_clear", 's');
                s->setValue("str_extra", 10);
                s->setValue("str_what", 'e');
                loadSettings();
        }
}

void MainWindow::gotoTab(int i) { ui->tabWidget->setCurrentIndex(i); }
