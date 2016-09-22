#ifndef SRC_TEXTS_LESSONMINER_H_
#define SRC_TEXTS_LESSONMINER_H_

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>
#include <QRegularExpression>
#include <QFile>


class LessonMiner : public QObject {
    Q_OBJECT

 public:
    explicit LessonMiner(QObject* parent = 0);
    ~LessonMiner();

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

#endif  // SRC_TEXTS_LESSONMINER_H_
