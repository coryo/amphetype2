#include "mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QFont>

void loadSettings()
{
        QSettings s;
        if (s.contains("typer_font")) {
                // std::cout<< "settings found" << std::endl;
        } else {
                // no settings found, write defaults
                QFont defaultFont("Consolas", -1);
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
                s.setValue("ana_which", "wpm asc");
                s.setValue("ana_what", 0);
                s.setValue("ana_many", 30);
                s.setValue("ana_count", 1);
                s.setValue("gen_copies", 3);
                s.setValue("gen_take", 2);
                s.setValue("gen_mix", 'c');
                s.setValue("str_clear", 's');
                s.setValue("str_extra", 10);
                s.setValue("str_what", 'e');
                s.setValue("typer_cols", 50);
                loadSettings();
        }        
}

int main(int argc, char *argv[])
{
        QCoreApplication::setOrganizationName("coryo");
        QCoreApplication::setOrganizationDomain("coryo.com");
        QCoreApplication::setApplicationName("Amphetype2");
        QSettings::setDefaultFormat(QSettings::IniFormat);

        //QSettings::setPath(QSettings::IniFormat, QSettings::UserScope);

        loadSettings();

        QApplication a(argc, argv);

        MainWindow w;

        w.show();

        return a.exec();
}
