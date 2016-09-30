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
    s.setValue("typer_font", defaultFont);
    s.setValue("history", 30.0);
    s.setValue("min_chars", 220);
    s.setValue("perf_group_by", 0);
    s.setValue("perf_items", -1);
    s.setValue("select_method", 0);
    s.setValue("num_rand", 50);
    s.setValue("show_last", true);
    s.setValue("show_xaxis", false);
    s.setValue("chrono_x", false);
    s.setValue("dampen_graph", false);
    s.setValue("dampen_average", 10);
    s.setValue("def_group_by", 10);
    s.setValue("ana_which", 0);
    s.setValue("ana_what", 0);
    s.setValue("ana_many", 30);
    s.setValue("ana_count", 1);
    s.setValue("typer_cols", 80);
    s.setValue("stylesheet", "basic");
    s.setValue("target_wpm", 80);
    s.setValue("target_acc", 97);
    s.setValue("target_vis", 1);
    s.setValue("perf_logging", true);
    s.setValue("liveplot_visible", true);
    s.setValue("debug_logging", false);
    s.setValue("keyboard_layout", 0);
    s.setValue("keyboard_standard", 0);
    loadSettings();
  }
}

int main(int argc, char *argv[]) {
  auto dest = QsLogging::DestinationFactory::MakeFileDestination(
      "amphetype2.log", QsLogging::LogRotationOption::EnableLogRotationOnOpen,
      QsLogging::MaxSizeBytes(1024 * 1024), QsLogging::MaxOldLogCount(1));

  QsLogging::Logger::instance().addDestination(dest);

  QLOG_INFO() << "Starting."
              << QString("v%1.%2.%3 build %4")
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
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::DebugLevel);
    QLOG_INFO() << "Debug Logging enabled.";
    QLOG_DEBUG() << "debug.";
  } else {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::InfoLevel);
    QLOG_INFO() << "Debug Logging disabled.";
    QLOG_DEBUG() << "debug.";
  }
  qInstallMessageHandler(logHandler);

  Database db;
  db.initDB();

  QFile file(":/stylesheets/" + s.value("stylesheet").toString() + ".qss");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    a.setStyleSheet(file.readAll());
    file.close();
  }

  MainWindow w;
  w.show();

  auto ret = a.exec();

  db.compress();

  return ret;
}
