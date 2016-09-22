#ifndef SRC_QUIZZER_TEST_H_
#define SRC_QUIZZER_TEST_H_

#include <QString>
#include <QVector>
#include <QList>
#include <QSet>
#include <QHash>
#include <QDateTime>
#include <QElapsedTimer>
#include <QObject>
#include <QKeyEvent>

#include "texts/text.h"


class Test : public QObject {
    Q_OBJECT

 public:
    explicit Test(Text*);
    ~Test();
    QHash<QPair<QChar, QChar>, int> getMistakes() const;
    double getFinalWpm() { return this->finalWPM; }
    double getFinalAccuracy() { return this->finalACC; }
    double getFinalViscosity() { return this->finalVIS; }

    void start();

    int mistakeCount() const;
    int msElapsed() const;
    double secondsElapsed() const;

    void handleInput(const QString&, int, Qt::KeyboardModifiers);

    Text* text;

    bool started;
    bool finished;
    int currentPos;
    QDateTime startTime;
    int totalMs;
    QVector<int> msBetween;
    QVector<int> timeAt;
    QVector<double> wpm;

    double minWPM;
    double minAPM;
    double maxWPM;
    double maxAPM;
    QSet<int> mistakes;

 private:
    void saveResult(const QString&, double, double, double);
    void finish();
    void addMistake(int, const QChar&, const QChar&);

    QList<QPair<QChar, QChar>> mistakeList;
    QElapsedTimer timer;
    QElapsedTimer intervalTimer;
    int apmWindow;

    double finalWPM;
    double finalACC;
    double finalVIS;

 signals:
    void testStarted(int);
    void done(double, double, double);
    void cancel(Test*);
    void restart(Test*);

    void newResult();
    void newStatistics();

    void mistake(int);
    void newWpm(double, double);
    void newApm(double, double);
    void characterAdded(int = 0, int = 0);
    void positionChanged(int, int);
};

#endif  // SRC_QUIZZER_TEST_H_
