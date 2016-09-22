#ifndef SRC_TEXTS_LESSONMINERCONTROLLER_H_
#define SRC_TEXTS_LESSONMINERCONTROLLER_H_

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QString>

#include "texts/lessonminer.h"


class LessonMinerController : public QObject {
    Q_OBJECT

 public:
    LessonMinerController() {
        miner = new LessonMiner();
        miner->moveToThread(&lessonMinerThread);
        connect(this,  &LessonMinerController::operate,
                miner, &LessonMiner::doWork);
        connect(miner, &LessonMiner::resultReady,
                this,  &LessonMinerController::handleResults);
        connect(miner, &LessonMiner::progress,
                this,  &LessonMinerController::progressUpdate);
        lessonMinerThread.start();
    }
    ~LessonMinerController() {
        lessonMinerThread.quit();
        lessonMinerThread.wait();
        delete miner;
    }

 private:
    LessonMiner* miner;
    QThread lessonMinerThread;

 public slots:
    void handleResults() { emit workDone(); }
    void progress(int p) { emit progressUpdate(p); }

 signals:
    void operate(const QString&);
    void workDone();
    void progressUpdate(int);
};

#endif  // SRC_TEXTS_LESSONMINERCONTROLLER_H_
