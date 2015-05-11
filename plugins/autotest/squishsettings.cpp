/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "squishsettings.h"

#include <QSettings>

namespace Autotest {
namespace Internal {

static const char group[] = "Squish";
static const char squishPathKey[] = "SquishPath";

void SquishSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(group));
    s->setValue(QLatin1String(squishPathKey), squishPath.toString());
    s->endGroup();
}

void SquishSettings::fromSettings(const QSettings *s)
{
    const QString root = QLatin1String(group) + QLatin1Char('/');
    squishPath = Utils::FileName::fromString(s->value(root + QLatin1String(squishPathKey)).toString());
}

bool SquishSettings::operator==(const SquishSettings &other) const
{
    return squishPath == other.squishPath;
}

bool SquishSettings::operator!=(const SquishSettings &other) const
{
    return !(*this == other);
}

} // namespace Internal
} // namespace Autotest
