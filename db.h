#ifndef DB_H
#define DB_H

#include <QVariantList>

#include "inc/sqlite3pp.h"
#include "inc/sqlite3ppext.h"

class QString;
class QStringList;

class DB
{
public:
        static QString db_path;

        static void initDB();
        static void addFunctions(sqlite3pp::database*);
        static int  getSource(const QString&, int = -1);

        // specific insertion functions
        static void addTexts(int, const QStringList&, int=-1, bool=true);
        static void addResult(const QString&, const QByteArray&, int, double, double, double);
        static void addStatistics(const QString&, const QMultiHash<QStringRef, double>&, 
                                  const QMultiHash<QStringRef, double>&, 
                                  const QMultiHash<QStringRef, int>&);
        static void addMistakes(const QString&, const QHash<QPair<QChar, QChar>, int>&);

        // specific query functions
        static void getMedianStats(int, double*, double*);
        static QList<QList<QString>> getSourcesData();
        static QList<QList<QString>> getTextsData(int);
        static QList<QList<QString>> getPerformanceData(int, int, int);
        static QList<QList<QString>> getSourcesList();
        static QList<QList<QString>> getStatisticsData(const QString&, int, int, const QString&, int);

private:
        // functions for sqlite extending 
        void med_step(sqlite3pp::ext::context&);
        void med_finalize(sqlite3pp::ext::context&);

        // general functions for retrieving data with a given query
        static QList<QString> getOneRow(const QString&);
        static QList<QList<QString>> getRows(const QString&);

        static void insertItems(const QString&, const QVariantList& values);
        static void insertItems(sqlite3pp::database*, const QString&, const QVariantList&);

signals:
        static void progress(int);
};

#endif // DB_H
