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

  if (s.value("debug_logging", false).toBool()) {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::DebugLevel);
    QLOG_INFO() << "Debug Logging enabled.";
    QLOG_DEBUG() << "debug.";
  } else {
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::Level::InfoLevel);
    QLOG_INFO() << "Debug Logging disabled.";
    QLOG_DEBUG() << "debug.";
  }
  qInstallMessageHandler(logHandler);

  QLOG_INFO() << qApp->applicationDirPath();
  QLOG_INFO() << QDir(".").absolutePath();
  Database db;
  db.initDB();

  QFile file(":/stylesheets/" + s.value("stylesheet", "basic").toString() + ".qss");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    a.setStyleSheet(file.readAll());
    file.close();
  }

  MainWindow w;
  w.show();

  auto ret = a.exec();

  return ret;
}
