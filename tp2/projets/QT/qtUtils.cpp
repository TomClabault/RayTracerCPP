#include "qtUtils.h"

int safe_text_to_int(const QString& text)
{
    bool ok;

    float value = text.toInt(&ok);

    if (!ok)
        return -1;
    else
        return value;
}

float safe_text_to_float(const QString& text)
{
    bool ok;

    float value = text.toFloat(&ok);

    if (!ok)
        return -1;
    else
        return value;
}
