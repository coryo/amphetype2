#ifndef PERFORMANCEHISTORY_H
#define PERFORMANCEHISTORY_H

#include <QWidget>
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

public:
        explicit PerformanceHistory(QWidget* parent = 0);
        ~PerformanceHistory();

private:
        Ui::PerformanceHistory* ui;
        QStandardItemModel* modelb;
        QCPGraph* dampen(QCPGraph*, int n = 10);

signals:
        void setText(Text*);
        void gotoTab(int);

private slots:
        void refreshSources();
        void refreshPerformance();
        void doubleClicked(const QModelIndex&);
        void showPlot(int=0);
        void writeSettings();
};

#endif // PERFORMANCEHISTORY_H
