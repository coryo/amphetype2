#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>

class QStandardItemModel;
class QSettings;

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QWidget
{
        Q_OBJECT

public:
        explicit StatisticsWidget(QWidget *parent = 0);
        ~StatisticsWidget();

private:
        Ui::StatisticsWidget *ui;
        QStandardItemModel* model;
        QSettings* s;

private slots:
        void refreshStatistics();
        void writeSettings();
        void resizeColumns();
};

#endif // STATISTICSWIDGET_H
