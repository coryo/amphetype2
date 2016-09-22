#ifndef SRC_QUIZZER_QUIZZER_H_
#define SRC_QUIZZER_QUIZZER_H_

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QColor>
#include <QString>
#include <QThread>
#include <QStringList>

#include "texts/textmanager.h"
#include "texts/text.h"
#include "quizzer/test.h"
#include "quizzer/typer.h"

namespace Ui {
class Quizzer;
}

class Quizzer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString goColor MEMBER goColor NOTIFY colorChanged)
    Q_PROPERTY(QString stopColor MEMBER stopColor NOTIFY colorChanged)

 public:
    explicit Quizzer(QWidget *parent = 0);
    ~Quizzer();
    Typer* getTyper() const;
    void alertText(const char *);

 private:
    Ui::Quizzer* ui;
    Text* text;
    QTimer lessonTimer;
    QTime lessonTime;
    QString goColor;
    QString stopColor;
    QThread testThread;

 signals:
    void wantText(Text*, SelectionMethod = SelectionMethod::None);
    void colorChanged();
    void newResult();
    void newStatistics();

    void newWpm(double, double);
    void newApm(double, double);
    void characterAdded(int = 0, int = 0);
    void testStarted(int);

 public slots:
    void setText(Text *);
    void tabActive(int);
    void setTyperFont();
    void updateTyperDisplay();

 private slots:
    void beginTest(int);
    void done(double, double, double);

    void setPreviousResultText(double, double);
    void cancelled(Test*);
    void restart(Test*);

    void timerLabelUpdate();
    void timerLabelReset();
    void timerLabelGo();
    void timerLabelStop();
};

#endif  // SRC_QUIZZER_QUIZZER_H_
