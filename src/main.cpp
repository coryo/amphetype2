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
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

#include <QsLog.h>
#include <QsLogDest.h>

#include "config.h"
#include "database/db.h"
#include "mainwindow/mainwindow.h"
#include "util/RunGuard.h"

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

int main(int argc, char *argv[]) {
  RunGuard guard("amphetype2");
  if (!guard.tryToRun()) return 0;

  QApplication a(argc, argv);

  // Application Setup
  QCoreApplication::setOrganizationName("coryo");
  QCoreApplication::setApplicationName("amphetype2");
  QCoreApplication::setApplicationVersion(amphetype2_VERSION_STRING_FULL);

  QDir appData(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
  appData.mkpath(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

  // QSettings
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     appData.absolutePath());

  // Logging
  auto dest = QsLogging::DestinationFactory::MakeFileDestination(
      appData.absolutePath() + "/amphetype2.log",
      QsLogging::LogRotationOption::EnableLogRotationOnOpen,
      QsLogging::MaxSizeBytes(1024 * 1024), QsLogging::MaxOldLogCount(1));

  QsLogging::Logger::instance().addDestination(dest);

  qInstallMessageHandler(logHandler);

  QLOG_INFO() << "Starting." << QCoreApplication::applicationVersion();
  QLOG_INFO() << "Application Data:" << appData.absolutePath();

  QSettings s;

  if (s.value("debug_logging", false).toBool()) {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::DebugLevel);
    QLOG_INFO() << "Debug Logging enabled.";
  } else {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::InfoLevel);
    QLOG_INFO() << "Debug Logging disabled.";
  }

  Database* db = new Database;
  db->initDB();
  delete db;

  QFile file(":/stylesheets/" + s.value("stylesheet", "basic").toString() +
             ".qss");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    a.setStyleSheet(file.readAll());
    file.close();
  }

  MainWindow w;
  w.show();

  auto ret = a.exec();

  return ret;
}
