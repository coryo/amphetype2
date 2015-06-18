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
        DB();
        ~DB();

        static QString db_path;

        static void initDB();
        static void addFunctions(sqlite3pp::database*);
        static int  getSource(const QString&, int = -1);
        static void getSourcesList(QList<QVariantList>*);
        static void addText(sqlite3pp::database*, int, const QString&, int=-1, bool=true);

private: 
        void med_step(sqlite3pp::ext::context&);
        void med_finalize(sqlite3pp::ext::context&);
};

#endif // DB_H
