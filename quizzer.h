#ifndef QUIZZER_H
#define QUIZZER_H

#include <QWidget>
#include <QTimer>

class Text;
class QSettings;

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

        int cursorPosition;
        QSettings* s;
        Ui::Quizzer* ui;
        Text* text;
        QTimer resizeTimer;

signals:
        void wantText();

private slots:
        void done();
        void setText(Text *);
        void setTyperFont();
        void tabActive(int);

        void moveCursor();

        // plot related slots
        void updatePlotRangeY(int, int = 0);
        void updatePlotRangeX(int, int = 0);
        void updatePlotWPM(double, double);
        void updatePlotAPM(double, double);
        void clearPlotData();
        void showLayers();
};

#endif // QUIZZER_H
