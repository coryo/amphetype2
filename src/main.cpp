#include <QApplication>
#include <QSettings>
#include <QFont>
#include <QDir>
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <QByteArray>

#include <QsLog.h>
#include <QsLogDest.h>

#include "database/db.h"
#include "mainwindow/mainwindow.h"
#include "config.h"

void logHandler(QtMsgType type, const QMessageLogContext &context,
        const QString &msg) {
  QByteArray localMsg = msg.toLocal8Bit();
  QString prefix;
  if (context.line) {
    prefix = QString("%1:%2:%3: ")
      .arg(context.file)
      .arg(context.line)
      .arg(context.function);
  }
  QString text = prefix + msg;
  switch (type) {
  case QtDebugMsg:
    QLOG_DEBUG() << text;
    break;
  case QtInfoMsg:
    QLOG_INFO() << text;
    break;
  case QtWarningMsg:
    QLOG_WARN() << text;
    break;
  case QtCriticalMsg:
    QLOG_ERROR() << text;
    break;
  case QtFatalMsg:
    QLOG_FATAL() << text;
    break;
  }
}

void loadSettings() {
  QSettings s;
  if (!s.contains("typer_font")) {
    // no settings found, write defaults
    QFont defaultFont("Consolas", 12);
    defaultFont.setStyleHint(QFont::Monospace);
    s.setValue("typer_font",         defaultFont);
    s.setValue("history",            30.0);
    s.setValue("min_chars",          220);
    // s.setValue("max_chars",          600);
    // s.setValue("lesson_stats",       0);
    s.setValue("perf_group_by",      0);
    s.setValue("perf_items",         -1);
    // s.setValue("text_regex",         "");
    s.setValue("db_name",            "db.sqlite");
    s.setValue("select_method",      0);
    s.setValue("num_rand",           50);
    // s.setValue("graph_what",         3);
    // s.setValue("req_space",          false);
    s.setValue("show_last",          true);
    s.setValue("show_xaxis",         false);
    s.setValue("chrono_x",           false);
    s.setValue("dampen_graph",       false);
    // s.setValue("minutes_in_sitting", 60.0);
    s.setValue("dampen_average",     10);
    s.setValue("def_group_by",       10);
    // s.setValue("use_lesson_stats",   false);
    // s.setValue("auto_review",        false);
    // s.setValue("min_wpm",            0.0);
    // s.setValue("min_acc",            0.0);
    // s.setValue("min_lesson_wpm",     0.0);
    // s.setValue("min_lesson_acc",     97.0);
    // s.setValue("group_month",        365.0);
    // s.setValue("group_week",         30.0);
    // s.setValue("group_day",          7.0);
    s.setValue("ana_which",          0);
    s.setValue("ana_what",           0);
    s.setValue("ana_many",           30);
    s.setValue("ana_count",          1);
    // s.setValue("gen_copies",         3);
    // s.setValue("gen_take",           2);
    // s.setValue("gen_mix",            'c');
    // s.setValue("str_clear",          's');
    // s.setValue("str_extra",          10);
    // s.setValue("str_what",           'e');
    s.setValue("typer_cols",         80);
    s.setValue("stylesheet",         "basic");
    s.setValue("target_wpm",         80);
    s.setValue("target_acc",         97);
    s.setValue("target_vis",         1);
    s.setValue("perf_logging",       true);
    s.setValue("liveplot_visible",   true);
    s.setValue("debug_logging",      false);
    loadSettings();
  }
}

int main(int argc, char *argv[]) {
  auto dest = QsLogging::DestinationFactory::MakeFileDestination(
    "amphetype2.log",
    QsLogging::LogRotationOption::EnableLogRotationOnOpen,
    QsLogging::MaxSizeBytes(1024 * 1024),
    QsLogging::MaxOldLogCount(1));

  QsLogging::Logger::instance().addDestination(dest);

  QLOG_INFO() << "Starting." << QString("v%1.%2.%3 build %4")
    .arg(amphetype2_VERSION_MAJOR)
    .arg(amphetype2_VERSION_MINOR)
    .arg(amphetype2_VERSION_MICRO)
    .arg(amphetype2_VERSION_BUILD);

  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("coryo");
  QCoreApplication::setOrganizationDomain("coryo.com");
  QCoreApplication::setApplicationName("amphetype2");
  QSettings::setDefaultFormat(QSettings::IniFormat);

  QSettings s;
  loadSettings();

  if (s.value("debug_logging").toBool()) {
    QsLogging::Logger::instance()
      .setLoggingLevel(QsLogging::Level::DebugLevel);
    QLOG_INFO() << "Debug Logging enabled.";
    QLOG_DEBUG() << "debug.";
  } else {
    QsLogging::Logger::instance()
      .setLoggingLevel(QsLogging::Level::InfoLevel);
    QLOG_INFO() << "Debug Logging disabled.";
    QLOG_DEBUG() << "debug.";
  }
  qInstallMessageHandler(logHandler);

  QString dir = qApp->applicationDirPath() + QDir::separator()
    + s.value("db_name").toString();
  DB::setDBPath(dir);
  DB::initDB();

  QFile file(":/stylesheets/" + s.value("stylesheet").toString() + ".qss");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    a.setStyleSheet(file.readAll());
    file.close();
  }

  MainWindow w;
  w.show();

  auto ret = a.exec();

  DB::compress();

  return ret;
}
