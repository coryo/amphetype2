#include "traininggenwidget.h"
#include "ui_traininggenwidget.h"
#include "traininggenerator.h"

#include "db.h"

#include <QString>
#include <QList>
#include <QChar>
#include <QDateTime>
#include <QDate>
TrainingGenWidget::TrainingGenWidget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::TrainingGenWidget)
{
        ui->setupUi(this);

        connect(ui->generateButton, SIGNAL(clicked()), this, SLOT(generate()));

        ui->layoutComboBox->addItem("QWERTY",  QVariant::fromValue(KeyboardLayout::QWERTY));
        ui->layoutComboBox->addItem("AZERTY",  QVariant::fromValue(KeyboardLayout::AZERTY));
        ui->layoutComboBox->addItem("QWERTZ",  QVariant::fromValue(KeyboardLayout::QWERTZ));
        ui->layoutComboBox->addItem("DVORAK",  QVariant::fromValue(KeyboardLayout::DVORAK));
        ui->layoutComboBox->addItem("COLEMAK", QVariant::fromValue(KeyboardLayout::COLEMAK));
        ui->layoutComboBox->addItem("WORKMAN", QVariant::fromValue(KeyboardLayout::WORKMAN)); 
}

TrainingGenWidget::~TrainingGenWidget()
{
        delete ui;
}

void TrainingGenWidget::generate()
{
        KeyboardLayout layout = ui->layoutComboBox->currentData().value<KeyboardLayout>();
        int lessonLength      = ui->lessonLengthSpinBox->value();
        int lessonsPerStage   = ui->lessonsPerStageSpinBox->value();
        
        TrainingGenerator tg(layout);

        QStringList bigList;
        QList<QStringList>* x = tg.generate(lessonsPerStage, lessonLength);
        for (QStringList l : *x) {
                for (QString s : l)
                        bigList.append(s);
        }

        QString layoutName;
        switch (layout) {
                case KeyboardLayout::QWERTY:  layoutName = "QWERTY";  break;
                case KeyboardLayout::AZERTY:  layoutName = "AZERTY";  break;
                case KeyboardLayout::QWERTZ:  layoutName = "QWERTZ";  break;
                case KeyboardLayout::COLEMAK: layoutName = "COLEMAK"; break;
                case KeyboardLayout::DVORAK:  layoutName = "DVORAK";  break;
                case KeyboardLayout::WORKMAN: layoutName = "WORKMAN"; break;
                default: return;
        }

        QDateTime now = QDateTime::currentDateTime();
        QString sourceName = layoutName+"::Generated Training " + now.toString("hh:mm:ss.zzz");

        int sourceId = DB::getSource(sourceName, 1);
        DB::addTexts(sourceId, bigList, 1);
        bigList.clear();

        delete x;
        emit generatedLessons();
}

