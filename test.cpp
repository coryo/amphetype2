#include "test.h"

Test::Test(const QString& t)
{
        text = t;
        length = text.length();
        currentPos = 0;
        editFlag = false;

        when.resize(text.length() + 1);
        timeBetween.resize(text.length());
        wpm.resize(text.length());
        apm.resize(text.length());
        //mistakeList.resize(text.length());

        maxWPM = 0;
        maxAPM = 0;
        minWPM = 300;
        minAPM = 300;
        apmWindow = 5;
}

Test::~Test() {}

QHash<QPair<QChar, QChar>, int> Test::getMistakes()
{
        QHash<QPair<QChar, QChar>, int> mis;

        for (QPair<QChar,QChar> value : mistakeMap.values()) {
                if (mis.contains(value))
                        mis.insert(value, mis.value(value)+1);
                else
                        mis.insert(value, 1);
        }
        return mis;
}
