#ifndef QUIZZER_H
#define QUIZZER_H

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QColor>
#include <QString>

class Text;
class QStringList;

namespace Ui {
class Quizzer;
}

class Quizzer : public QWidget {
        Q_OBJECT
        Q_PROPERTY(QColor wpmLineColor MEMBER wpmLineColor NOTIFY colorChanged)
        Q_PROPERTY(QColor apmLineColor MEMBER apmLineColor NOTIFY colorChanged)
        Q_PROPERTY(QColor plotBackgroundColor MEMBER plotBackgroundColor NOTIFY colorChanged)
        Q_PROPERTY(QColor plotForegroundColor MEMBER plotForegroundColor NOTIFY colorChanged)
        Q_PROPERTY(QString goColor MEMBER goColor NOTIFY colorChanged)
        Q_PROPERTY(QString stopColor MEMBER stopColor NOTIFY colorChanged)

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

        QColor wpmLineColor;
        QColor apmLineColor;
        QColor plotBackgroundColor;
        QColor plotForegroundColor;
        QString goColor;
        QString stopColor;

signals:
        void wantText(Text*);
        void colorChanged();

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
        void updateColors();

        void timerLabelUpdate();
        void timerLabelReset();
        void timerLabelGo();
        void timerLabelStop();
        
};

#endif // QUIZZER_H
