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

void SquishSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(group));
    s->endGroup();
}

void SquishSettings::fromSettings(const QSettings *s)
{
}

bool SquishSettings::operator==(const SquishSettings &other) const
{
    return true;
}

bool SquishSettings::operator!=(const SquishSettings &other) const
{
    return !(*this == other);
}

} // namespace Internal
} // namespace Autotest
