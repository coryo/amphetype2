#include "db.h"

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QCryptographicHash>
//#include <QSqlQuery>
//#include <QSqlError>
//#include <QtSql>

#include <iostream>
//#include <QSqlDriver>
#include "inc/sqlite3pp.h"

#include <string>
#include <vector>
#include <algorithm>

DB::DB() {}

struct agg_median
{
        void step(double x) {
                l.push_back(x);
        }
        double finish() {
                if (l.empty()) return 0;
                std::sort(l.begin(), l.end());
                double median;
                int length = l.size();
                if (length % 2 == 0)
                        median = (l[length / 2] + l[length / 2 - 1]) / 2;
                else
                        median = l[length / 2];
                return median;
        }
        std::vector<double> l;
};

void DB::addFunctions(sqlite3pp::database* db)
{
        sqlite3pp::ext::aggregate aggr(*db);
        aggr.create<agg_median, double>("agg_median");  
}

sqlite3pp::database* DB::openDB()
{
        QSettings s;
        QString dir = qApp->applicationDirPath()
                + QDir::separator()
                + s.value("db_name").toString();
        sqlite3pp::database* db;
        
        db = new sqlite3pp::database(dir.toStdString().c_str());  

        return db;
}

void DB::initDb2()
{
        try {
                sqlite3pp::database* db = DB::openDB();
                db->execute("create table source (name text, disabled integer, "
                    "discount integer)");
                db->execute("create table text (id text primary key, source integer, "
                    "text text, disabled integer)");
                db->execute("create table result (w real, text_id text, source "
                    "integer, wpm real, accuracy real, viscosity real)");
                db->execute("create table statistic (w real, data text, type integer, "
                    "time real, count integer, mistakes integer, viscosity "
                    "real)");
                db->execute("create table mistake (w real, target text, mistake text, "
                            "count integer)");
                db->execute("create view text_source as select "
                            "id,s.name,text,coalesce(t.disabled,s.disabled) from text "
                            "as t left join source as s on (t.source = s.rowid)");             
        } catch (std::exception const& e) {
                std::cout << "db error" <<std::endl;
                std::cout << e.what() <<std::endl;
        }
}

/*
QSqlError DB::initDb(const QString& db_name)
{
        QSettings s;

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(qApp->applicationDirPath()
                           + QDir::separator()
                           + s.value("db_name").toString());
        if (!db.open())
                return db.lastError();

        QStringList tables = db.tables();
        if (tables.contains("source", Qt::CaseInsensitive) &&
                tables.contains("text", Qt::CaseInsensitive)) {
                return QSqlError();
        }

        QSqlQuery q;

        // make the tables
        if (!q.exec("create table source (name text, disabled integer, "
                    "discount integer)"))
                return q.lastError();
        if (!q.exec("create table text (id text primary key, source integer, "
                    "text text, disabled integer)"))
                return q.lastError();
        if (!q.exec("create table result (w real, text_id text, source "
                    "integer, wpm real, accuracy real, viscosity real)"))
                return q.lastError();
        if (!q.exec("create table statistic (w real, data text, type integer, "
                    "time real, count integer, mistakes integer, viscosity "
                    "real)"))
                return q.lastError();
        if (!q.exec("create table mistake (w real, target text, mistake text, "
                    "count integer)"))
                return q.lastError();
        if (!q.exec("create view text_source as select "
                    "id,s.name,text,coalesce(t.disabled,s.disabled) from text "
                    "as t left join source as s on (t.source = s.rowid)"))
                return q.lastError();

        return QSqlError();
}*/

int DB::getSource(const QString& source, int lesson)
{
        std::cout << "getsource " << std::endl;
        sqlite3pp::database* db = DB::openDB();
        QString query("select rowid from source where name = \""+source+"\" limit 1");
        std::cout << "getsource1 " << std::endl;
        std::cout << query.toStdString() <<std::endl;
        sqlite3pp::query qry(*db, query.toStdString().c_str());
        std::cout << "getsource2 " << std::endl;
        QByteArray rowid;
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                for (int j = 0; j < qry.column_count(); ++j) {
                        rowid = (*i).get<char const*>(j);
                        //(*i).getter() >> rowid;
                }
        }
        std::cout << rowid.data() << std::endl;
        if (!rowid.isEmpty()) {
                sqlite3pp::command cmd(*db, "select rowid from source where name = :name limit 1");
                cmd.bind(":name", rowid.data());
                cmd.execute();
                delete db;
                return rowid.toInt();
        }
        sqlite3pp::command cmd(*db, "insert into source (name, discount) values (?, ?)");
        std::string src = source.toStdString();
        cmd.bind(1, src.c_str());
        std::cout << source.toLocal8Bit().data() <<std::endl;
        if (lesson == -1)
                cmd.bind(2);
        else
                cmd.bind(2, lesson);
        cmd.execute();
        delete db;
        return getSource(source, lesson);
        
        /*
        QSqlQuery q;
        q.prepare("select rowid from source where name = ? limit 1");
        q.addBindValue(source);
        q.exec();
        q.first();

        if (q.isValid()) {
                int rowid = q.value(0).toInt();
                q.prepare("update source set disabled = NULL where rowid = ?");
                q.addBindValue(rowid);
                q.exec();
                return rowid;
        }
        q.prepare("insert into source (name,discount) values (?,?)");
        q.addBindValue(source);
        if (lesson == -1)
                q.addBindValue(QVariant(QVariant::String));
        else
                q.addBindValue(lesson);
        q.exec();
        return getSource(source, lesson);*/
}

void DB::addTexts(int source, const QString& text, int lesson, bool update)
{
        QByteArray txt_id = 
                QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1);
        txt_id = txt_id.toHex();
        int dis = ((lesson == 2) ? 1 : 0);
        try {
                std::cout <<"add text"<<std::endl;
                sqlite3pp::database* db = DB::openDB();
                QString query("insert into text (id,text,source,disabled) "
                          "values (:id,:text,:source,:disabled)");
                sqlite3pp::command cmd(*db, query.toStdString().c_str());
                cmd.bind(":id", txt_id.data());
                std::string src = text.toStdString();
                cmd.bind(":text", src.c_str());
                cmd.bind(":source", source);
                if (dis == 0)
                        cmd.bind(":disabled", "");
                else
                        cmd.bind(":disabled", dis);
                cmd.execute();
                delete db;              
                /*
                QSqlQuery q;
                q.prepare("insert into text (id,text,source,disabled) "
                          "values (:id,:text,:source,:disabled)");
                q.bindValue(":id", txt_id);
                q.bindValue(":text", text);
                q.bindValue(":source", source);
                if (dis == 0)
                        q.bindValue(":disabled", QVariant::String);
                else
                        q.bindValue(":disabled", dis);
                q.exec();*/
        } catch (std::exception& e) {
                //
        }
}

void DB::getSourcesList(QList<QVariantList>* s)
{
        sqlite3pp::database* db = DB::openDB();
        sqlite3pp::query qry(*db, "select rowid,name from source order by name");
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                QVariantList source;
                source << (*i).get<char const*>(0);
                source << (*i).get<char const*>(1);
                (*s) <<source;
        }


        /*
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.open())
                return;

        QSqlQuery q;
        q.prepare("select rowid,name from source order by name");
        q.exec();
        while(q.next()) {
                QVariantList source;
                source << q.value(0) << q.value(1);
                (*s) << source;
        }*/
}
