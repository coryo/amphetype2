#ifndef LESSONMINER_H
#define LESSONMINER_H

#include <QObject>
#include <QList>
#include <QStringList>

class QRegularExpression;
class QFile;
class QString;

class LessonMiner : public QObject
{
        Q_OBJECT

public:
        explicit LessonMiner(QObject* parent = 0);

private:
        void fileToParagraphs(QFile*, QList<QStringList>*);
        QStringList sentenceSplitter(const QString&);
        void makeLessons(const QList<QStringList>&, QStringList*);
        QString popFormat(QStringList*);

private:
        QStringList abbr;
        int min_chars;

signals:
        void progress(int);
        void resultReady();

public slots:
        void doWork(const QString&);
};

#endif // LESSONMINER_H
