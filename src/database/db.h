#ifndef SRC_DATABASE_DB_H_
#define SRC_DATABASE_DB_H_

#include <sqlite3pp.h>
#include <sqlite3ppext.h>

#include <QVariantList>
#include <QThread>
#include <QObject>
#include <QMutex>

#include "texts/text.h"
#include "quizzer/test.h"


class DBConnection {
 public:
    explicit DBConnection(const QString&);
    ~DBConnection();
    sqlite3pp::database& getDB();
 private:
    sqlite3pp::database* db;
    sqlite3pp::ext::function* func;
    sqlite3pp::ext::aggregate* aggr;
};

class DB : public QObject {
    Q_OBJECT

 public:
    static void initDB();

    static void setDBPath(const QString&);

    // specific insertion functions
    static void addText(int, const QString&, int = -1, bool = true);
    static void addTexts(int, const QStringList&, int = -1, bool = true);
    static void addResult(const QString&, const Text*, double,
                                double, double);
    static void addStatistics(const QString&,
                                const QMultiHash<QStringRef, double>&,
                                const QMultiHash<QStringRef, double>&,
                                const QMultiHash<QStringRef, int>&);
    static void addMistakes(const QString&, const Test*);

    // removal/modification
    static void deleteSource(const QList<int>&);
    static void deleteText(const QList<int>&);
    static void deleteResult(const QString&, const QString&);
    static void disableSource(const QList<int>&);
    static void enableSource(const QList<int>&);

    // specific query functions
    static int getSource(const QString&, int lesson = -1, int type = 0);
    static QPair<double, double> getMedianStats(int);
    static QList<QStringList> getSourcesData();
    static QList<QStringList> getTextsData(int);
    static QList<QStringList> getPerformanceData(int, int, int);
    static QList<QStringList> getSourcesList();
    static QList<QStringList> getStatisticsData(const QString&, int, int,
                                                int, int);
    // create a text object
    static Text* getNextText();  // get the text that follows the last completed text
    static Text* getNextText(Text*);  // get the text that follows the given text
    static Text* getRandomText();
    static Text* getText(int);  // get a text with a given rowid
    static Text* getText(const QString&);  // get a text with a given hashid

    static void updateText(int, const QString&);
    static void compress();

 private:
    static QMutex db_lock;
    static QString db_path;

    // general functions for retrieving data with a given query
    static QStringList getOneRow(const char *);
    static QStringList getOneRow(const char *, QVariantList&);
    static QStringList getOneRow(const char *, QVariant);
    static QStringList getOneRow(sqlite3pp::database&, const char *);
    static QStringList getOneRow(sqlite3pp::database&, const char *,
                                 QVariantList&);
    static QStringList getOneRow(sqlite3pp::database&, const char *,
                                 QVariant);
    // static QList<QStringList> getRows(const QString&);
    static QList<QStringList> getRows(const char *);
    static QList<QStringList> getRows(const char *, QVariant);
    static QList<QStringList> getRows(const char *, QVariantList&);
    // general functions for executing commands
    static void bindAndRun(sqlite3pp::command*, const QVariantList&);
    static void bindAndRun(sqlite3pp::command*, const QVariant&);
    static void bindAndRun(const QString&, const QVariantList&);
    static void bindAndRun(const QString&, const QVariant&);
    static void binder(sqlite3pp::statement*, const QVariantList&);
    // create a text object with a given query
    static Text* getTextWithQuery(const QString&);
    static void createFunctions(sqlite3pp::database*, sqlite3pp::ext::function*,
                                sqlite3pp::ext::aggregate*);

 signals:
    void progress(int);
};

#endif  // SRC_DATABASE_DB_H_
