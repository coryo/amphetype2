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

#ifndef SRC_TEXTS_EDITTEXTDIALOG_H_
#define SRC_TEXTS_EDITTEXTDIALOG_H_

#include <QDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialogButtonBox>

class EditTextDialog : public QDialog {
  Q_OBJECT

 public:
  explicit EditTextDialog(const QString& title, const QString& text,
                          QWidget* parent = Q_NULLPTR);

 public slots:
  void verify();

 public:
  QString text() const;

 private:
  QLabel* text_label_;
  QPlainTextEdit* text_edit_;
  QDialogButtonBox* button_box_;
};

#endif  // SRC_TEXTS_EDITTEXTDIALOG_H_
