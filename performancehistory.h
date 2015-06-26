#ifndef PERFORMANCEHISTORY_H
#define PERFORMANCEHISTORY_H

#include <QWidget>
#include <QColor>

#include "inc/qcustomplot.h"

class QModelIndex;
class Text;
class QStandardItemModel;

namespace Ui
{
class PerformanceHistory;
}

class PerformanceHistory : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor wpmLineColor        MEMBER wpmLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor smaLineColor        MEMBER smaLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor targetLineColor     MEMBER targetLineColor     NOTIFY colorChanged)
        Q_PROPERTY(QColor plotBackgroundColor MEMBER plotBackgroundColor NOTIFY colorChanged)
        Q_PROPERTY(QColor plotForegroundColor MEMBER plotForegroundColor NOTIFY colorChanged)

public:
        explicit PerformanceHistory(QWidget* parent = 0);
        ~PerformanceHistory();

private:
        Ui::PerformanceHistory* ui;
        QStandardItemModel* model;
        QCPGraph* dampen(QCPGraph*, int n = 10);

        QColor wpmLineColor;
        QColor smaLineColor;
        QColor targetLineColor;
        QColor plotBackgroundColor;
        QColor plotForegroundColor;

signals:
        void setText(Text*);
        void gotoTab(int);
        void colorChanged();
        void settingsChanged();

private slots:
        void refreshSources();
        void refreshPerformance();
        void doubleClicked(const QModelIndex&);
        void showPlot(int=0);
        void refreshCurrentPlot();
        void writeSettings();
        void updateColors();
};

#endif // PERFORMANCEHISTORY_H
