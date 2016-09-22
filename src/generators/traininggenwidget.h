#ifndef SRC_GENERATORS_TRAININGGENWIDGET_H_
#define SRC_GENERATORS_TRAININGGENWIDGET_H_

#include <QWidget>

namespace Ui {
class TrainingGenWidget;
}

class TrainingGenWidget : public QWidget {
    Q_OBJECT

 public:
    explicit TrainingGenWidget(QWidget *parent = 0);
    ~TrainingGenWidget();

 private:
    Ui::TrainingGenWidget *ui;

 signals:
    void generatedLessons();

 public slots:
    void generate();
};

#endif  // SRC_GENERATORS_TRAININGGENWIDGET_H_
