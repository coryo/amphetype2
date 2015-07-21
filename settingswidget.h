#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class SettingsWidget;
}

class SettingsWidget : public QWidget
{
        Q_OBJECT

public:
        explicit SettingsWidget(QWidget *parent = 0);
        ~SettingsWidget();

private:
        Ui::SettingsWidget *ui;

signals:
        void settingsChanged();

private slots:
        void selectFont();
        void changeStyleSheet(int);
        void changePerfLogging(int);
        void writeTargets();
};

#endif // SETTINGSWIDGET_H
