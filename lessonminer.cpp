#include "lessonminer.h"

#include "db.h"
#include "inc/sqlite3pp.h"

#include <iostream>

#include <QSettings>
#include <QString>
#include <QStringRef>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QCryptographicHash>

LessonMiner::LessonMiner(QObject *parent)
        : QObject(parent)
{
        QSettings s;
        min_chars = s.value("min_chars").toInt();

        // things to ignore as sentence enders.
        // ie "Mr. Smith." is shouldn't be 2 sentences
        abbr    <<"jr"<<"mr"<<"mrs"<<"ms"<<"dr"<<"prof"<<"sr"<<"sen"<<"rep"
                <<"sens"<<"reps"<<"gov"<<"attys"<<"atty"<<"supt"<<"det"
                <<"rev"<<"col"<<"gen"<<"lt"<<"cmdr"<<"adm"<<"capt"<<"sgt"
                <<"cpl"<<"maj"<<"dept"<<"univ"<<"assn"<<"bros"<<"inc"<<"ltd"
                <<"co"<<"corp"<<"arc"<<"al"<<"ave"<<"blvd"<<"bld"<<"cl"
                <<"ct"<<"cres"<<"dr"<<"expy"<<"exp"<<"dist"<<"mt"<<"ft"
                <<"fwy"<<"fy"<<"hway"<<"hwy"<<"la"<<"pde"<<"pd"<<"pl"
                <<"plz"<<"rd"<<"st"<<"tce"<<"Ala"<<"Ariz"<<"Ark"<<"Cal"
                <<"Calif"<<"Col"<<"Colo"<<"Conn"<<"Del"<<"Fed"<<"Fla"<<"Ga"
                <<"Ida"<<"Id"<<"Ill"<<"Ind"<<"Ia"<<"Kan"<<"Kans"<<"Ken"
                <<"Ky"<<"La"<<"Me"<<"Md"<<"Is"<<"Mass"<<"Mich"<<"Minn"
                <<"Miss"<<"Mo"<<"Mont"<<"Neb"<<"Nebr"<<"Nev"<<"Mex"<<"Okla"
                <<"Ok"<<"Ore"<<"Penna"<<"Penn"<<"Pa"<<"Dak"<<"Tenn"<<"Tex"
                <<"Ut"<<"Vt"<<"Va"<<"Wash"<<"Wis"<<"Wisc"<<"Wy"<<"Wyo"
                <<"USAFA"<<"Alta"<<"Man"<<"Ont"<<"Qué"<<"Sask"<<"Yuk"<<"jan"
                <<"feb"<<"mar"<<"apr"<<"may"<<"jun"<<"jul"<<"aug"<<"sep"
                <<"oct"<<"nov"<<"dec"<<"sept"<<"vs"<<"etc"<<"no"<<"esp"
                <<"eg"<<"ie"<<"1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"<<"8"<<"9"
                <<"10"<<"11"<<"12"<<"avg"<<"viz"<<"m"<<"mme";
}

LessonMiner::~LessonMiner() { }

void LessonMiner::doWork(const QString& fname)
{
        // open the file
        QFile file(fname);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return;

        // parse the file into a list of lists. each QStringList is a paragraph,
        // each QString in the QStringList is a sentence.
        QList<QStringList> paragraphs;
        fileToParagraphs(&file, &paragraphs);

        // turn that list into lessons
        QStringList lessons;
        makeLessons(paragraphs, &lessons);

        // add the lessons to the database
        QFileInfo fi(fname);
        int id = DB::getSource(fi.fileName(), -1);
        double i = 0.0;

        sqlite3pp::database db(DB::db_path.toStdString().c_str());// = DB::openDB();
        sqlite3pp::transaction xct(db);
        {
                for (QString& x : lessons) {
                        addTexts(&db, id, x, -1, false);
                        i += 1.0;
                        // value from 0 to 100 for a progress bar
                        emit progress((int)(100 * (i / lessons.size())));
                }
        }
        xct.commit();

        // done
        emit resultReady();
}

void LessonMiner::addTexts(sqlite3pp::database* db, int source, const QString& text, int lesson, bool update)
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


void LessonMiner::fileToParagraphs(QFile* f, QList<QStringList>* paragraphs)
{
        QStringList paragraph;
        QTextStream in(f);

        while (!in.atEnd()) {
                // QTextStream readline trims leading/trailing whitespace
                QString line = in.readLine();

                if (!line.isEmpty()) {
                        // add the line to the current paragraph
                        paragraph << line;
                } else if (paragraph.size() > 0) {
                        // paragraph is done, dump it into the list.
                        *paragraphs << sentenceSplitter(paragraph.join(" "));
                        paragraph.clear();
                }
        }
        if (paragraph.size() > 0) {
                *paragraphs << sentenceSplitter(paragraph.join(" "));
        }
}

// Splits a QString representing a paragraph into a QStringList of its sentences
QStringList LessonMiner::sentenceSplitter(const QString& text)
{
        QRegularExpression re("(?:(?: |^)[^\\w. ]*(?P<pre>\\w+)[^ "
                              ".]*\\.+|[?!]+)['\"]?(?= +(?:[^ a-z]|$))|$");

        int start = 0;
        QStringList list;

        QRegularExpressionMatchIterator i = re.globalMatch(text);
        while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                // if there is a pre match, and it's an abbreviation, skip to
                // the next
                // match because it's not a valid sentence ending
                bool a = match.captured("pre").isNull();
                bool b = abbr.contains(match.captured("pre"),
                                       Qt::CaseInsensitive);
                if (!a && b)
                        continue;

                // the position of the character at the end of the match
                int end = match.capturedEnd();
                // add substring from start to end, to the list
                list << QStringRef(&text, start, end - start)
                                .toString()
                                .trimmed();
                start = end;
        }

        return list;
}

// turn the split paragraphs/sentences into lessons
void LessonMiner::makeLessons(const QList<QStringList>& pgs,
                              QStringList* lessons)
{
        lessons->clear();
        QStringList backlog;
        int backlen = 0;
        double i = 0.0;
        QList<QStringList>::const_iterator p;
        for (p = pgs.constBegin(); p != pgs.constEnd(); ++p) {
                if (backlog.size() > 0)
                        backlog << QString("");
                QStringList::const_iterator s;
                for (s = (*p).constBegin(); s != (*p).constEnd(); ++s) {
                        backlog << (*s);
                        backlen += (*s).length();
                        if (backlen >= min_chars) {
                                (*lessons) << popFormat(&backlog);
                                backlen = 0;
                        }
                }
                i += 1.0;
                emit progress((int)(100 * (i / pgs.size())));
        }
        if (backlen > 0)
                (*lessons) << popFormat(&backlog);
}

// joins the backlog list into a qstring, with \n between sentences of different
// paragraphs
QString LessonMiner::popFormat(QStringList* lst)
{
        QStringList ret;
        QStringList p;

        while (lst->size() > 0) {
                QString s = lst->first();
                lst->pop_front();
                if (!s.isEmpty()) {
                        p << s;
                } else {
                        ret << p.join(" ");
                        p.clear();
                }
        }
        if (p.size() > 0)
                ret << p.join(" ");

        return ret.join("\n");
}