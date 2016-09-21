#ifndef DB_H
#define DB_H

#include <sqlite3pp.h>
#include <sqlite3ppext.h>

#include <QVariantList>
#include <QThread>
#include <QObject>
#include <QMutex>

class Text;
class Test;


class DB : public QObject
{
        Q_OBJECT
public:
        static void initDB();

        static void setDBPath(const QString&);

        // specific insertion functions
        static void addText      (int, const QString&,     int = -1, bool = true);
        static void addTexts     (int, const QStringList&, int = -1, bool = true);
        static void addResult    (const QString&, const Text*, double,
                                  double, double);
        static void addStatistics(const QString&,
                                  const QMultiHash<QStringRef, double>&,
                                  const QMultiHash<QStringRef, double>&,
                                  const QMultiHash<QStringRef, int>&);
        static void addMistakes  (const QString&,
                                  const Test*);

        // removal/modification
        static void deleteSource (const QList<int>&);
        static void deleteText   (const QList<int>&);
        static void deleteResult (const QString&, const QString&);
        static void disableSource(const QList<int>&);
        static void enableSource (const QList<int>&);

        // specific query functions
        static int                   getSource(const QString&, int lesson=-1, int type=0);
        static QPair<double, double> getMedianStats(int);
        static QList<QStringList>    getSourcesData();
        static QList<QStringList>    getTextsData(int);
        static QList<QStringList>    getPerformanceData(int, int, int);
        static QList<QStringList>    getSourcesList();
        static QList<QStringList>    getStatisticsData(const QString&, int, int, int, int);
        // create a text object
        static Text* getNextText();             // get the text that follows the last completed text
        static Text* getNextText(Text*);        // get the text that follows the given text
        static Text* getRandomText();
        static Text* getText(int);              // get a text with a given rowid
        static Text* getText(const QString&);   // get a text with a given hashid

        static void updateText(int, const QString&);
        static void compress();

private:
        static QMutex db_lock;
        static QString db_path;
        // functions for sqlite extending
        static int counter();
        static int _count;
        // general functions for retrieving data with a given query
        static QStringList getOneRow(const QString&, bool = false);
        static QStringList getOneRow(sqlite3pp::database*, const QString&, bool=false);
        static QList<QStringList> getRows(const QString&, bool = false);
        // general functions for executing commands
        static void execCommand(const QString&);
        static void execCommand(sqlite3pp::database*,
                                const QString&);
        static void insertItems(const QString&, const QVariantList&);
        static void insertItems(sqlite3pp::database*,
                                const QString&, const QVariantList&);
        // create a text object with a given query
        static Text* getTextWithQuery(const QString&);
        static void createFunctions(sqlite3pp::database*, sqlite3pp::ext::function*, sqlite3pp::ext::aggregate*);

signals:
        void progress(int);
};

#endif // DB_H
