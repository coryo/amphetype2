#ifndef TRAININGGENWIDGET_H
#define TRAININGGENWIDGET_H

#include <QWidget>

namespace Ui {
class TrainingGenWidget;
}

class TrainingGenWidget : public QWidget
{
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

#endif // TRAININGGENWIDGET_H
