#include "db.h"

#include <QString>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>
#include <QtSql>

#include <iostream>

DB::DB()
{
}

QSqlError DB::initDb(const QString& db_name)
{
        QSettings s("Amphetype2.ini", QSettings::IniFormat);
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(db_name);
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
}

int DB::getSource(const QString& source, int lesson)
{
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
        return getSource(source, lesson);
}

void DB::addTexts(int source, const QString& text, int lesson, bool update)
{
        QByteArray txt_id = QCryptographicHash::hash(text.toUtf8(),
                                                     QCryptographicHash::Sha1);
        txt_id = txt_id.toHex();
        int dis = ((lesson == 2) ? 1 : 0);
        try {
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
                q.exec();
        } catch (std::exception& e) {
                //
        }
}

void DB::getSourcesList(QList<QVariantList>* s)
{
        QSqlQuery q;
        q.prepare("select rowid,name from source order by name");
        q.exec();
        while(q.next()) {
                QVariantList source;
                source << q.value(0) << q.value(1);
                (*s) << source;
        }
}