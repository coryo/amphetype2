#ifndef LESSONMINERCONTROLLER
#define LESSONMINERCONTROLLER

#include "lessonminer.h"

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QString>

class LessonMinerController : public QObject
{
    Q_OBJECT
    QThread lessonMinerThread;
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

public slots:
        void handleResults() { emit workDone(); }
        void progress(int p) { emit progressUpdate(p); }

signals:
        void operate (const QString&);
        void workDone();
        void progressUpdate(int);
};

#endif // LESSONMINERCONTROLLER

