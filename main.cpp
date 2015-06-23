#include "mainwindow.h"

#include <QApplication>
#include <QSettings>
#include <QFont>
#include <QDir>
#include <QString>

#include "db.h"

void loadSettings()
{
        QSettings s;
        if (s.contains("typer_font")) {
                // std::cout<< "settings found" << std::endl;
        } else {
                // no settings found, write defaults
                QFont defaultFont("Consolas", 12);
                defaultFont.setStyleHint(QFont::Monospace);
                s.setValue("typer_font", defaultFont);
                s.setValue("history", 30.0);
                s.setValue("min_chars", 220);
                s.setValue("max_chars", 600);
                s.setValue("lesson_stats", 0);
                s.setValue("perf_group_by", 0);
                s.setValue("perf_items", 100);
                s.setValue("text_regex", "");
                s.setValue("db_name", "db.sqlite");
                s.setValue("select_method", 0);
                s.setValue("num_rand", 50);
                s.setValue("graph_what", 3);
                s.setValue("req_space", false);
                s.setValue("show_last", true);
                s.setValue("show_xaxis", false);
                s.setValue("chrono_x", false);
                s.setValue("dampen_graph", false);
                s.setValue("minutes_in_sitting", 60.0);
                s.setValue("dampen_average", 10);
                s.setValue("def_group_by", 10);
                s.setValue("use_lesson_stats", false);
                s.setValue("auto_review", false);
                s.setValue("min_wpm", 0.0);
                s.setValue("min_acc", 0.0);
                s.setValue("min_lesson_wpm", 0.0);
                s.setValue("min_lesson_acc", 97.0);
                s.setValue("quiz_right_fg", "#000000");
                s.setValue("quiz_right_bg", "#ffffff");
                s.setValue("quiz_wrong_fg", "#ffffff");
                s.setValue("quiz_wrong_bg", "#000000");
                s.setValue("group_month", 365.0);
                s.setValue("group_week", 30.0);
                s.setValue("group_day", 7.0);
                s.setValue("ana_which", 0);
                s.setValue("ana_what", 0);
                s.setValue("ana_many", 30);
                s.setValue("ana_count", 1);
                s.setValue("gen_copies", 3);
                s.setValue("gen_take", 2);
                s.setValue("gen_mix", 'c');
                s.setValue("str_clear", 's');
                s.setValue("str_extra", 10);
                s.setValue("str_what", 'e');
                s.setValue("typer_cols", 80);
                loadSettings();
        }        
}

int main(int argc, char *argv[])
{
        QApplication a(argc, argv);

        QCoreApplication::setOrganizationName("coryo");
        QCoreApplication::setOrganizationDomain("coryo.com");
        QCoreApplication::setApplicationName("Amphetype2");
        QSettings::setDefaultFormat(QSettings::IniFormat);

        loadSettings();

        QSettings s;
        QString dir = qApp->applicationDirPath()
                + QDir::separator()
                + s.value("db_name").toString();
        DB::setDBPath(dir);    

        DB::initDB();

        QString style = 
                "* { background-color: #333333; color: #FFFFFF; }"
                // Tab widget
                "QTabWidget::pane { border-top: 2px solid #555555; }"
                "QTabWidget::tab-bar { left: 5px; }"
                "QTabBar::tab { min-width: 15ex; border: 2px solid #444444; border-top-left-radius: 6px; border-top-right-radius: 6px; padding: 3px; border-bottom-color: #555555; background: #555555; }"
                "QTabBar::tab:selected { background: #995555; }"
                "QTabBar::tab:!selected { margin-top: 4px; }"
                // Horizontal scrollbars
                "QScrollBar:horizontal { border: 2px solid grey; background: #333333; height: 15px; margin: 0px 15px 0px 15px; }"
                "QScrollBar::handle:horizontal { background: #995555; min-width: 20px; }"
                "QScrollBar::add-line:horizontal { border: 2px solid grey; background: #333333; width: 15px; subcontrol-position: right; subcontrol-origin: margin; }"
                "QScrollBar::sub-line:horizontal { border: 2px solid grey; background: #333333; width: 15px; subcontrol-position: left; subcontrol-origin: margin; }"
                "QScrollBar:left-arrow:horizontal, QScrollBar::right-arrow:horizontal { width: 3px; height: 3px; background: #FFFFFF; }"
                "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
                // Vertical scrollbars
                "QScrollBar:vertical { border: 2px solid grey; background: #333333; width: 15px; margin: 15px 0px 15px 0px; }"
                "QScrollBar::handle:vertical { background: #995555; min-height: 20px; }"
                "QScrollBar::add-line:vertical { border: 2px solid grey; background: #333333; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; }"
                "QScrollBar::sub-line:vertical { border: 2px solid grey; background: #333333; height: 15px; subcontrol-position: top; subcontrol-origin: margin; }"
                "QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { width: 3px; height: 3px; background: #FFFFFF; }"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
                // Table views
                "QTableView { border: 2px solid grey; background-color: #444444; alternate-background-color: #555555; selection-background-color: #995555; }"
                "QTableView QTableCornerButton::section { background: #995555; }"
                // the header view of tables
                "QHeaderView::section { background: #995555; border: 0px; }"

                "QTextEdit, QLineEdit, QComboBox, QSpinBox { padding: 2px; border: 2px solid gray; background: #444444; selection-background-color: #444444; }"
                "QTextEdit { border: 0px; }"

                "QPushButton { padding: 2px; border: 2px solid #8f8f91; background-color: #444444; }"

                "QCheckBox::indicator { border: 2px solid grey; width: 7px; height: 7px; }"
                "QCheckBox::indicator:unchecked { background: #555555; }"
                "QCheckBox::indicator:checked { background: #995555; }"

                "TyperDisplay { color: #FFFFFF; qproperty-correctColor: #79B221; qproperty-errorColor: #995555; }"
                // plot colors of the typer tab
                "Quizzer {"
                        "qproperty-wpmLineColor: rgb(121, 178, 33);"
                        "qproperty-apmLineColor: rgba(214, 187, 187, 80);"
                        "qproperty-plotBackgroundColor: rgb(51,51,51);"
                        "qproperty-plotForegroundColor: rgb(255,255,255); }"
                // plot colors of the performance tab
                "PerformanceHistory {"
                        "qproperty-wpmLineColor: rgba(121, 178, 33,150);"
                        "qproperty-smaLineColor: rgb(121, 178, 33);"
                        "qproperty-plotBackgroundColor: rgb(51,51,51);"
                        "qproperty-plotForegroundColor: rgb(255,255,255); }";
        a.setStyleSheet(style);

        MainWindow w;

        w.show();

        return a.exec();
}
