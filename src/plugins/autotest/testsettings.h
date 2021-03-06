/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef TESTSETTINGS_H
#define TESTSETTINGS_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

enum MetricsType {
    Walltime,
    TickCounter,
    EventCounter,
    CallGrind,
    Perf
};

struct TestSettings
{
    TestSettings();
    void toSettings(QSettings *s) const;
    void fromSettings(const QSettings *s);
    bool equals(const TestSettings &rhs) const;
    static QString metricsTypeToOption(const MetricsType type);

    int timeout;
    MetricsType metrics;
    bool omitInternalMssg;
    bool omitRunConfigWarn;
    bool limitResultOutput;
    bool autoScroll;
    bool alwaysParse;
};

inline bool operator==(const TestSettings &s1, const TestSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const TestSettings &s1, const TestSettings &s2) { return !s1.equals(s2); }

} // namespace Internal
} // namespace Autotest

#endif // TESTSETTINGS_H
