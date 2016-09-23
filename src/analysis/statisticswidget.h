#ifndef SRC_ANALYSIS_STATISTICSWIDGET_H_
#define SRC_ANALYSIS_STATISTICSWIDGET_H_

#include <QWidget>
#include <QStringList>
#include <QStandardItemModel>

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QWidget {
  Q_OBJECT

 public:
  explicit StatisticsWidget(QWidget *parent = 0);
  ~StatisticsWidget();

 private:
  Ui::StatisticsWidget *ui;
  QStandardItemModel* model;

 signals:
  void newItems(QStringList&);

 public slots:
  void populateStatistics();

 private slots:
  void writeSettings();
  void generateList();
};

#endif  // SRC_ANALYSIS_STATISTICSWIDGET_H_
