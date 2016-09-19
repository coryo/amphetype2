#ifndef PERFORMANCEHISTORY_H
#define PERFORMANCEHISTORY_H

#include <QWidget>
#include <QColor>

class QModelIndex;
class Text;
class QStandardItemModel;
class QCPGraph;

namespace Ui
{
class PerformanceHistory;
}

class PerformanceHistory : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor wpmLineColor        MEMBER wpmLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor accLineColor        MEMBER accLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor visLineColor        MEMBER visLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor smaLineColor        MEMBER smaLineColor        NOTIFY colorChanged)
        Q_PROPERTY(QColor targetLineColor     MEMBER targetLineColor     NOTIFY colorChanged)
        Q_PROPERTY(QColor plotBackgroundColor MEMBER plotBackgroundColor NOTIFY colorChanged)
        Q_PROPERTY(QColor plotForegroundColor MEMBER plotForegroundColor NOTIFY colorChanged)

public:
        explicit PerformanceHistory(QWidget* parent = 0);
        ~PerformanceHistory();

private:
        void contextMenu(const QPoint &);
        QCPGraph* dampen(QCPGraph*, int n = 10);

        Ui::PerformanceHistory* ui;
        QStandardItemModel*     model;
        // colors
        QColor wpmLineColor;
        QColor accLineColor;
        QColor visLineColor;
        QColor smaLineColor;
        QColor targetLineColor;
        QColor plotBackgroundColor;
        QColor plotForegroundColor;

signals:
        void setText(Text*);
        void gotoTab(int);
        void colorChanged();
        void settingsChanged();

public slots:
        void refreshPerformance();
        void refreshCurrentPlot();

private slots:
        void deleteResult(bool);
        void refreshSources();
        void doubleClicked(const QModelIndex&);
        void showPlot(int=0);
        void writeSettings();
        void updateColors();
        void togglePlotLine(int);
};

#endif // PERFORMANCEHISTORY_H
