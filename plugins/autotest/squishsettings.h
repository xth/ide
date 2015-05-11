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

#ifndef SQUISHSETTINGS_H
#define SQUISHSETTINGS_H

#include <utils/fileutils.h>

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

struct SquishSettings
{
    void toSettings(QSettings *s) const;
    void fromSettings(const QSettings *s);

    bool operator==(const SquishSettings &other) const;
    bool operator!=(const SquishSettings &other) const;

    Utils::FileName squishPath;
};


} // namespace Internal
} // namespace Autotest

#endif // SQUISHSETTINGS_H
