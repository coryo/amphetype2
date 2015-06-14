#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QFontDialog>
#include <QFont>
#include <QDir>
#include <QSettings>

SettingsWidget::SettingsWidget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::SettingsWidget),
        s(new QSettings(qApp->applicationDirPath() + QDir::separator() +
                                  "Amphetype2.ini",
                          QSettings::IniFormat))
{
        ui->setupUi(this);
        ui->fontLabel->setFont(qvariant_cast<QFont>(s->value("typer_font")));

        connect(ui->fontButton, SIGNAL(pressed()), this, SLOT(selectFont()));

}

SettingsWidget::~SettingsWidget()
{
        delete ui;
}

void SettingsWidget::selectFont()
{
        bool ok;
        QFont font = QFontDialog::getFont(&ok, qvariant_cast<QFont>(s->value("typer_font")));

        if (ok) {
                s->setValue("typer_font", font);
                ui->fontLabel->setFont(font);
                emit settingsChanged();
        } else {

        }
}
