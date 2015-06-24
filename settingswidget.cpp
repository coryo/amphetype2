#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QFontDialog>
#include <QFont>
#include <QDir>
#include <QSettings>

SettingsWidget::SettingsWidget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::SettingsWidget)
{
        ui->setupUi(this);

        QSettings s;

        ui->fontLabel->setFont(qvariant_cast<QFont>(s.value("typer_font")));

        ui->styleSheetComboBox->addItem("Dark Theme", "dark-1.qss");
        ui->styleSheetComboBox->addItem("Basic Theme", "empty.qss");

        connect(ui->fontButton, SIGNAL(pressed()), this, SLOT(selectFont()));
        connect(ui->styleSheetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeStyleSheet(int)));
}

SettingsWidget::~SettingsWidget()
{
        delete ui;
}

void SettingsWidget::changeStyleSheet(int i)
{
        QString ss = ui->styleSheetComboBox->itemData(i, Qt::UserRole).toString();

        QFile file(":/stylesheets/"+ss);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
               qApp->setStyleSheet(file.readAll());
               file.close(); 
        }
}

void SettingsWidget::selectFont()
{
        QSettings s;
        bool ok;
        QFont font = QFontDialog::getFont(&ok, qvariant_cast<QFont>(s.value("typer_font")));

        if (ok) {
                s.setValue("typer_font", font);
                ui->fontLabel->setFont(font);
                emit settingsChanged();
        } else {

        }
}
