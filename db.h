#ifndef DB_H
#define DB_H

//#include <QByteArray>
//#include <QList>
#include <QSqlError>
#include <QVariantList>

class QString;
class QStringList;

class DB
{
public:
        DB();
        static QSqlError initDb(const QString&);
        static int getSource(const QString&, int = -1);
        static void addTexts(int, const QString&, int = -1, bool = true);
        static void getSourcesList(QList<QVariantList>*);
};

#endif // DB_H
