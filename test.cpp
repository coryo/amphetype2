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

        mistake.resize(text.length());

        maxWPM = 0;
        maxAPM = 0;
        minWPM = 300;
        minAPM = 300;

        apmWindow = 5;
}

Test::~Test() {}
