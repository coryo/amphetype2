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

#include "texts/edittextdialog.h"

#include <QVBoxLayout>

EditTextDialog::EditTextDialog(const QString& title,
                               QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f) {
  text_label_ = new QLabel(tr("Text:"));

  text_edit_ = new QPlainTextEdit;
  //text_edit_->setPlainText(text);

  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  connect(button_box_, SIGNAL(accepted()), this, SLOT(verify()));
  connect(button_box_, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(text_label_);
  mainLayout->addWidget(text_edit_);
  mainLayout->addWidget(button_box_);
  setLayout(mainLayout);

  setWindowTitle(title);
}

QString EditTextDialog::text() const { return text_edit_->toPlainText(); }
void EditTextDialog::setText(const QString& text) { text_edit_->setPlainText(text); }

void EditTextDialog::verify() { accept(); }