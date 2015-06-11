#ifndef PERFORMANCEHISTORY_H
#define PERFORMANCEHISTORY_H

#include <QWidget>
#include "inc/qcustomplot.h"
class TreeModel;
class QModelIndex;
class Text;
class QSettings;
//class QCPDataMap;

namespace Ui
{
class PerformanceHistory;
}

class PerformanceHistory : public QWidget
{
        Q_OBJECT

public:
        explicit PerformanceHistory(QWidget* parent = 0);
        ~PerformanceHistory();

private:
        Ui::PerformanceHistory* ui;
        TreeModel* model;
        QSettings* s;
        QCPGraph* dampen(QCPGraph*, int n = 10);

signals:
        void setText(Text*);
        void gotoTab(int);

private slots:
        void refreshSources();
        void refreshPerformance();
        void doubleClicked(const QModelIndex&);
        void resizeColumns();
        void showPlot(int=0);
        //void changeTimeScaled(int);
        //void changeFullRangeY(int);
        void writeSettings();
        //void dampenGraph(int);
};

#endif // PERFORMANCEHISTORY_H
