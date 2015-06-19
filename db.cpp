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
        try {
                sqlite3pp::ext::aggregate aggr(*db);
                aggr.create<agg_median, double>("agg_median"); 
        } catch (std::exception& e) {
                std::cout << e.what() << std::endl;
        } 
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
        } catch (std::exception& e) {
                std::cout << "cannot create database" <<std::endl;
                std::cout << e.what() <<std::endl;
        }
}

int DB::getSource(const QString& source, int lesson)
{
        try {
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
                        cmd.bind(2);
                else
                        cmd.bind(2, lesson);
                cmd.execute();
                return getSource(source, lesson);
        } catch (std::exception& e) {
                std::cout << "error getting source" << std::endl;
                std::cout << e.what() << std::endl;
                return -1;
        }
}


void DB::addText(sqlite3pp::database* db, int source, const QString& text, int lesson, bool update)
{
        QByteArray txt_id = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1);
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
                std::cout << "error adding text" << std::endl;
                std::cout << e.what() << std::endl;
        }
}

void DB::getSourcesList(QList<QVariantList>* s)
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                sqlite3pp::query qry(db, "select rowid,name from source order by name");
                for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                        QVariantList source;
                        source << (*i).get<char const*>(0);
                        source << (*i).get<char const*>(1);
                        (*s) << source;
                }
        }
        catch (std::exception& e) {
                std::cout << "error populating sources" << std::endl;
                std::cout << e.what() << std::endl;
        }
}

void DB::addResult(const char* time, const char* id, int source, double wpm, double acc, double vis)
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                sqlite3pp::transaction resultTransaction(db);
                {
                        sqlite3pp::command cmd(db, "insert into result (w,text_id,source,wpm,accuracy,viscosity)"
                                                " values (:time,:id,:source,:wpm,:accuracy,:viscosity)");

                        cmd.bind(":time",      time);
                        cmd.bind(":id",        id);
                        cmd.bind(":source",    source);
                        cmd.bind(":wpm",       wpm);
                        cmd.bind(":accuracy",  acc);
                        cmd.bind(":viscosity", vis);
                        cmd.execute();
                }
                resultTransaction.commit();
        } catch (std::exception& e) {
                std::cout << "error adding result" << std::endl;
                std::cout << e.what() << std::endl;
        }
}

void DB::addStatistics(const char* time, const QMultiHash<QStringRef, double>& stats, 
                       const QMultiHash<QStringRef, double>& visc, 
                       const QMultiHash<QStringRef, int>& mistakeCount)
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                sqlite3pp::transaction statisticsTransaction(db);
                {
                        QList<QStringRef> keys = stats.uniqueKeys();
                        
                        for (QStringRef k : keys) {
                                sqlite3pp::command cmd(db, "insert into statistic (time,viscosity,w,count,mistakes,type,data) "
                          "values (:time,:visc,:when,:count,:mistakes,:type,:data)");

                                const QList<double>& timeValues = stats.values(k);
                                if (timeValues.length() > 1)
                                        cmd.bind(":time", ((timeValues[timeValues.length()/2] + (timeValues[timeValues.length()/2 - 1]))/2.0));
                                else if (timeValues.length() == 1)
                                        cmd.bind(":time", timeValues[timeValues.length()/2]);
                                else
                                        cmd.bind(":time", timeValues.first());

                                // get the median viscosity
                                const QList<double>& viscValues = visc.values(k);
                                if (viscValues.length() > 1)
                                        cmd.bind(":visc", ((viscValues[viscValues.length()/2]+viscValues[viscValues.length()/2-1])/2.0) * 100.0);
                                else if (viscValues.length() == 1)
                                        cmd.bind(":visc", viscValues[viscValues.length()/2] * 100.0);
                                else
                                        cmd.bind(":visc", viscValues.first() * 100.0);

                                cmd.bind(":when",     time);
                                cmd.bind(":count",    stats.count(k));
                                cmd.bind(":mistakes", mistakeCount.count(k));

                                if (k.length() == 1)
                                        cmd.bind(":type", 0);
                                else if (k.length() == 3)
                                        cmd.bind(":type", 1);
                                else 
                                        cmd.bind(":type", 2);

                                std::string _k(k.toString().toStdString());
                                cmd.bind(":data", _k.c_str());
                                cmd.execute();
                        }
                }
                statisticsTransaction.commit();
        } catch (std::exception& e) {
                std::cout << "error adding statistics" << std::endl;
                std::cout << e.what() << std::endl;
        }
}

void DB::addMistakes(const char* time, const QHash<QPair<QChar, QChar>, int>& m)
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                sqlite3pp::transaction mistakesTransaction(db);
                {
                        QHashIterator<QPair<QChar,QChar>, int> it(m);

                        while (it.hasNext()) {
                                it.next();
                                sqlite3pp::command cmd(db, "insert into mistake (w,target,mistake,count) "
                                        "values (:time,:target,:mistake,:count)");

                                QString a(it.key().first);
                                QString b(it.key().second);

                                std::string aa = a.toStdString();
                                std::string bb = b.toStdString();

                                cmd.bind(":time",    time);
                                cmd.bind(":target",  aa.c_str());
                                cmd.bind(":mistake", bb.c_str());
                                cmd.bind(":count",   it.value());
                                cmd.execute();
                        }
                }
                mistakesTransaction.commit();
        } catch (std::exception& e) {
                std::cout << "error adding mistakes" << std::endl;
                std::cout << e.what() << std::endl;
        }
}

 void DB::getMedianStats(int n, double* wpm,double* acc)
 {
        QSettings s;
        QString query = "select agg_median(wpm), agg_median(acc) from "
                "(select wpm,100.0*accuracy as acc from result "
                "order by datetime(w) desc limit "+QString::number(n)+")";

        QList<QByteArray> cols = getOneRow(query);

        *wpm = cols[0].toDouble();
        *acc = cols[1].toDouble();
 }

QList<QByteArray> DB::getOneRow(const QString& sql)
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                DB::addFunctions(&db);

                sqlite3pp::query qry(db, sql.toStdString().c_str());
                QList<QByteArray> row;
                for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                        for (int j = 0; j < qry.column_count(); ++j)
                                row << (*i).get<char const*>(j);
                }
                return row;
        } catch (std::exception& e) {
                std::cout << "error running query" << std::endl;
                std::cout << e.what() << std::endl;
                return QList<QByteArray>();
        }
}

QList<QList<QByteArray>> DB::getRows(const QString& sql)
{
        try {
                sqlite3pp::database db(DB::db_path.toStdString().c_str());
                DB::addFunctions(&db);

                sqlite3pp::query qry(db, sql.toStdString().c_str());
                QList<QList<QByteArray>> rows;
                QList<QByteArray> row;
                for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                        for (int j = 0; j < qry.column_count(); ++j)
                                row << (*i).get<char const*>(j);
                        rows << row;
                }
                return rows;
        } catch (std::exception& e) {
                std::cout << "error running query" << std::endl;
                std::cout << e.what() << std::endl;
                return QList<QList<QByteArray>>();
        }        
}