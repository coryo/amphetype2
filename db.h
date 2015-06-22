#ifndef DB_H
#define DB_H

#include <QVariantList>

#include "inc/sqlite3pp.h"
#include "inc/sqlite3ppext.h"

class QString;
class QStringList;
class Text;

class DB
{
public:
        static QString db_path;

        static void initDB();
        static void addFunctions(sqlite3pp::database*);
        static int  getSource(const QString&, int = -1);

        // specific insertion functions
        static void addText(int, const QString&, int=-1, bool=true);
        static void addTexts(int, const QStringList&, int=-1, bool=true);
        static void addResult(const QString&, const QByteArray&, int, double, double, double);
        static void addStatistics(const QString&, const QMultiHash<QStringRef, double>&, 
                                  const QMultiHash<QStringRef, double>&, 
                                  const QMultiHash<QStringRef, int>&);
        static void addMistakes(const QString&, const QHash<QPair<QChar, QChar>, int>&);

        static void deleteSource(const QList<int>&);
        static void deleteText(const QList<int>&);
        static void disableSource(const QList<int>&);
        static void enableSource(const QList<int>&);

        // specific query functions
        static std::pair<double,double> getMedianStats(int);
        static QList<QStringList> getSourcesData();
        static QList<QStringList> getTextsData(int);
        static QList<QStringList> getPerformanceData(int, int, int);
        static QList<QStringList> getSourcesList();
        static QList<QStringList> getStatisticsData(const QString&, int, int, int, int);
        // create a text object 
        static Text* getNextText(int);          // get next text based on given select method
        static Text* getText(int);              // get a text with a given rowid
        static Text* getText(const QString&);   // get a text with a given hashid

private:
        // functions for sqlite extending 
        void med_step(sqlite3pp::ext::context&);
        void med_finalize(sqlite3pp::ext::context&);

        // general functions for retrieving data with a given query
        static QStringList getOneRow(const QString&);
        static QList<QStringList> getRows(const QString&);

        static void execCommand(sqlite3pp::database*, const QString&);
        static void execCommand(const QString&);

        static void insertItems(const QString&, const QVariantList& values);
        static void insertItems(sqlite3pp::database*, const QString&, const QVariantList&);
        // create a text object with a given query
        static Text* getTextWithQuery(const QString&);

signals:
        static void progress(int);
};

#endif // DB_H
