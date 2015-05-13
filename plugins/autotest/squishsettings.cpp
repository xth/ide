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
static const char licensePathKey[] = "LicensePath";
static const char localKey[] = "Local";
static const char serverHostKey[] = "ServerHost";
static const char serverPortKey[] = "ServerPort";
static const char verboseKey[] = "Verbose";

void SquishSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(group));
    s->setValue(QLatin1String(squishPathKey), squishPath.toString());
    s->setValue(QLatin1String(licensePathKey), licensePath.toString());
    s->setValue(QLatin1String(localKey), local);
    s->setValue(QLatin1String(serverHostKey), serverHost);
    s->setValue(QLatin1String(serverPortKey), serverPort);
    s->setValue(QLatin1String(verboseKey), verbose);
    s->endGroup();
}

void SquishSettings::fromSettings(const QSettings *s)
{
    const QString root = QLatin1String(group) + QLatin1Char('/');
    squishPath = Utils::FileName::fromString(s->value(root + QLatin1String(squishPathKey)).toString());
    licensePath = Utils::FileName::fromString(s->value(root + QLatin1String(licensePathKey)).toString());
    local = s->value(root + QLatin1String(localKey), true).toBool();
    serverHost = s->value(root + QLatin1String(serverHostKey), QStringLiteral("localhost")).toString();
    serverPort = s->value(root + QLatin1String(serverPortKey), 9999).toUInt();
    verbose = s->value(root + QLatin1String(verboseKey), false).toBool();
}

bool SquishSettings::operator==(const SquishSettings &other) const
{
    return local == other.local
            && verbose == other.verbose
            && serverPort == other.serverPort
            && squishPath == other.squishPath
            && licensePath == other.licensePath
            && serverHost == other.serverHost;
}

bool SquishSettings::operator!=(const SquishSettings &other) const
{
    return !(*this == other);
}

} // namespace Internal
} // namespace Autotest
