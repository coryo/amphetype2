#ifndef DB_H
#define DB_H

#include <QSqlError>
#include <QVariantList>

#include "inc/sqlite3pp.h"
#include "inc/sqlite3ppext.h"
class QString;
class QStringList;

class DB
{
public:
        DB();
        ~DB();
        static QSqlError initDb(const QString&);
        static int getSource(const QString&, int = -1);
        static void addTexts(int, const QString&, int = -1, bool = true);
        static void getSourcesList(QList<QVariantList>*);

        static void addFunctions(sqlite3pp::database*);
        static sqlite3pp::database* openDB(const QString&);

private:
        void med_step(sqlite3pp::ext::context&);
        void med_finalize(sqlite3pp::ext::context&);
};

#endif // DB_H
