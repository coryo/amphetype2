#include "generators/traininggenwidget.h"

#include <QString>
#include <QList>
#include <QChar>
#include <QDateTime>
#include <QDate>

#include "ui_traininggenwidget.h"
#include "generators/traininggenerator.h"
#include "database/db.h"

TrainingGenWidget::TrainingGenWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::TrainingGenWidget) {
  ui->setupUi(this);

  connect(ui->generateButton, SIGNAL(clicked()), this, SLOT(generate()));

  ui->layoutComboBox->addItem("QWERTY",
                              QVariant::fromValue(Amphetype::Layout::QWERTY));
  ui->layoutComboBox->addItem("AZERTY",
                              QVariant::fromValue(Amphetype::Layout::AZERTY));
  ui->layoutComboBox->addItem("QWERTZ",
                              QVariant::fromValue(Amphetype::Layout::QWERTZ));
  ui->layoutComboBox->addItem("DVORAK",
                              QVariant::fromValue(Amphetype::Layout::DVORAK));
  ui->layoutComboBox->addItem("COLEMAK",
                              QVariant::fromValue(Amphetype::Layout::COLEMAK));
  ui->layoutComboBox->addItem("WORKMAN",
                              QVariant::fromValue(Amphetype::Layout::WORKMAN));
}

TrainingGenWidget::~TrainingGenWidget() { delete ui; }

void TrainingGenWidget::generate() {
  Amphetype::Layout layout =
      ui->layoutComboBox->currentData().value<Amphetype::Layout>();
  int lessonLength = ui->lessonLengthSpinBox->value();
  int lessonsPerStage = ui->lessonsPerStageSpinBox->value();

  TrainingGenerator tg(layout);

  QStringList bigList;
  QList<QStringList>* x = tg.generate(lessonsPerStage, lessonLength);
  for (QStringList l : *x) {
    for (QString s : l) bigList.append(s);
  }

  QString layoutName;
  switch (layout) {
    case Amphetype::Layout::QWERTY:
      layoutName = "QWERTY";
      break;
    case Amphetype::Layout::AZERTY:
      layoutName = "AZERTY";
      break;
    case Amphetype::Layout::QWERTZ:
      layoutName = "QWERTZ";
      break;
    case Amphetype::Layout::COLEMAK:
      layoutName = "COLEMAK";
      break;
    case Amphetype::Layout::DVORAK:
      layoutName = "DVORAK";
      break;
    case Amphetype::Layout::WORKMAN:
      layoutName = "WORKMAN";
      break;
    default:
      return;
  }

  QDateTime now = QDateTime::currentDateTime();
  QString sourceName =
      layoutName + "::Generated Training " + now.toString("hh:mm:ss.zzz");

  Database db;
  int sourceId = db.getSource(sourceName, 1, 1);
  db.addTexts(sourceId, bigList, 1);
  bigList.clear();

  delete x;
  // emit generatedLessons();
  emit newTraining();
}
