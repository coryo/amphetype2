#include "test.h"

Test::Test(const QString& t)
{
        text = t;
        length = text.length();
        currentPos = 0;
        editFlag = false;

        msBetween.resize(text.length());
        wpm.resize(text.length());
        apm.resize(text.length());

        maxWPM = 0;
        maxAPM = 0;
        minWPM = DBL_MAX;
        minAPM = DBL_MAX;
        apmWindow = 5;
}

QHash<QPair<QChar, QChar>, int> Test::getMistakes()
{
        QHash<QPair<QChar, QChar>, int> mis;

        for (QPair<QChar, QChar> value : mistakeMap.values()) {
                if (mis.contains(value))
                        mis.insert(value, mis.value(value)+1);
                else
                        mis.insert(value, 1);
        }
        // ((targetChar, mistakenChar), count)
        return mis;
}
