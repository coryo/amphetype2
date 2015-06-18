#include "db.h"

#include "inc/sqlite3pp.h"

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QCryptographicHash>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

DB::DB() {}

QString DB::db_path = QString();

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

void DB::initDB()
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                db.execute("create table source (name text, disabled integer, "
                    "discount integer)");
                db.execute("create table text (id text primary key, source integer, "
                    "text text, disabled integer)");
                db.execute("create table result (w real, text_id text, source "
                    "integer, wpm real, accuracy real, viscosity real)");
                db.execute("create table statistic (w real, data text, type integer, "
                    "time real, count integer, mistakes integer, viscosity "
                    "real)");
                db.execute("create table mistake (w real, target text, mistake text, "
                            "count integer)");
                db.execute("create view text_source as select "
                            "id,s.name,text,coalesce(t.disabled,s.disabled) from text "
                            "as t left join source as s on (t.source = s.rowid)");           
        } catch (std::exception const& e) {
                std::cout << "db error" <<std::endl;
                std::cout << e.what() <<std::endl;
        }
}

int DB::getSource(const QString& source, int lesson)
{
        sqlite3pp::database db(DB::db_path.toStdString().c_str());
        QString query("select rowid from source where name = \""+source+"\" limit 1");

        sqlite3pp::query qry(db, query.toStdString().c_str());

        QByteArray rowid;
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
                rowid = (*i).get<char const*>(0);

        if (!rowid.isEmpty()) {
                sqlite3pp::command cmd(db, "select rowid from source where name = :name limit 1");
                cmd.bind(":name", rowid.data());
                cmd.execute();
                return rowid.toInt();
        }
        sqlite3pp::command cmd(db, "insert into source (name, discount) values (?, ?)");
        std::string src = source.toStdString();
        cmd.bind(1, src.c_str());

        if (lesson == -1)
                cmd.bind(2, SQLITE_NULL);
        else
                cmd.bind(2, lesson);
        cmd.execute();
        return getSource(source, lesson);
}


void DB::addText(sqlite3pp::database* db, int source, const QString& text, int lesson, bool update)
{
        QByteArray txt_id = 
                QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1);
        txt_id = txt_id.toHex();
        int dis = ((lesson == 2) ? 1 : 0);
        try {
                QString query("insert into text (id,text,source,disabled) "
                          "values (:id,:text,:source,:disabled)");
                sqlite3pp::command cmd(*db, query.toStdString().c_str());
                cmd.bind(":id", txt_id.data());
                std::string src = text.toStdString();
                cmd.bind(":text", src.c_str());
                cmd.bind(":source", source);
                if (dis == 0)
                        cmd.bind(":disabled");
                else
                        cmd.bind(":disabled", dis);
                cmd.execute();          
        } catch (std::exception& e) {
                //
        }
}

void DB::getSourcesList(QList<QVariantList>* s)
{
        sqlite3pp::database db(DB::db_path.toStdString().c_str());
        sqlite3pp::query qry(db, "select rowid,name from source order by name");
        for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                QVariantList source;
                source << (*i).get<char const*>(0);
                source << (*i).get<char const*>(1);
                (*s) << source;
        }
}
