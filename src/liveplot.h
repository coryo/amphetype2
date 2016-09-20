#ifndef LIVEPLOT_H
#define LIVEPLOT_H

#include <qcustomplot.h>

#define APM_PLOT 0
#define WPM_PLOT 1


class LivePlot;

class LivePlot : public QCustomPlot {
        Q_OBJECT
        Q_PROPERTY(QColor wpmLineColor        MEMBER wpmLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor apmLineColor        MEMBER apmLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor targetLineColor     MEMBER targetLineColor     NOTIFY colorChanged)
        Q_PROPERTY(QColor plotBackgroundColor MEMBER plotBackgroundColor NOTIFY colorChanged)
        Q_PROPERTY(QColor plotForegroundColor MEMBER plotForegroundColor NOTIFY colorChanged)

public:
        LivePlot(QWidget* parent=0);
private:
        // colors
        QColor  wpmLineColor;
        QColor  apmLineColor;
        QColor  targetLineColor;
        QColor  plotBackgroundColor;
        QColor  plotForegroundColor;
        QString goColor;
        QString stopColor;

public slots:
        void beginTest(int);
        void newKeyPress(int, int);
        void updatePlotRangeY(int = 0, int = 0);
        void updatePlotRangeX(int = 0, int = 0);
        void addWpm(double, double);
        void addApm(double, double);
        void clearPlotData();
        void showGraphs();
        void setPlotVisible(int);
        void updatePlotTargetLine();
        void updateColors();

signals:
        void colorChanged();
};

#endif // LIVEPLOT_H