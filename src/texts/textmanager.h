#ifndef SRC_TEXTS_TEXTMANAGER_H_
#define SRC_TEXTS_TEXTMANAGER_H_

#include <QWidget>
#include <QModelIndex>
#include <QStandardItemModel>

#include "texts/lessonminercontroller.h"
#include "texts/text.h"

namespace Ui {
class TextManager;
}

enum class SelectionMethod {None = -1, Random = 0, InOrder, Repeat, Ask};
Q_DECLARE_METATYPE(SelectionMethod);

class TextManager : public QWidget {
  Q_OBJECT

 public:
  explicit TextManager(QWidget* parent = 0);
  ~TextManager();

 private:
  Ui::TextManager* ui;
  QStringList files;
  LessonMinerController* lmc;
  bool refreshed;
  QStandardItemModel* sourcesModel;
  QStandardItemModel* textsModel;

 signals:
  void setText(Text*);
  void progress(int);
  void gotoTab(int);
  void sourceDeleted();

 public slots:
  void nextText(Text*, SelectionMethod method = SelectionMethod::None);
  void tabActive(int);
  void refreshSources();

 private slots:
  void addFiles();
  void doubleClicked(const QModelIndex&);
  void processNextFile();
  void changeSelectMethod(int);
  void populateTexts(const QModelIndex&);
  void addSource();
  void deleteSource();
  void addText();
  void deleteText();
  void enableSource();
  void disableSource();
  void toggleTextsWidget();
  void editText();
};

#endif  // SRC_TEXTS_TEXTMANAGER_H_
