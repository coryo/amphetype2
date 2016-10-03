// Copyright (C) 2016  Cory Parsons
//
// This file is part of amphetype2.
//
// amphetype2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// amphetype2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with amphetype2.  If not, see <http://www.gnu.org/licenses/>.
//

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
  void newTraining(int);

 public slots:
  void generate();
};

#endif  // SRC_GENERATORS_TRAININGGENWIDGET_H_
