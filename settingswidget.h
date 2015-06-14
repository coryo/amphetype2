#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

class QSettings;

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
        QSettings* s;
        Ui::SettingsWidget *ui;

signals:
        void settingsChanged();

private slots:
        void selectFont();
};

#endif // SETTINGSWIDGET_H
