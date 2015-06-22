#ifndef QUIZZER_H
#define QUIZZER_H

#include <QWidget>
#include <QTimer>
#include <QTime>

class Text;
class QStringList;

namespace Ui {
class Quizzer;
}

class Quizzer : public QWidget {
        Q_OBJECT

public:
        explicit Quizzer(QWidget *parent = 0);
        ~Quizzer();

private:
        void resizeEvent(QResizeEvent*);

        Ui::Quizzer* ui;
        Text*  text;
        QTimer resizeTimer;
        QTimer lessonTimer;
        QTime  lessonTime;

signals:
        void wantText(Text*);

private slots:
        void done();
        void setText(Text *);
        void setTyperFont();
        void tabActive(int);
        void setPreviousResultText(double, double);
        void cancelled();

        // plot related slots
        void updatePlotRangeY(int, int = 0);
        void updatePlotRangeX(int, int = 0);
        void addPlotPoint(int, double, double);
        void clearPlotData();
        void showGraphs();
        void setPlotVisible(int);

        void timerLabelUpdate();
        void timerLabelReset();
        void timerLabelGo();
        void timerLabelStop();
};

#endif // QUIZZER_H
