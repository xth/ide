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

#include "testsettings.h"

#include <QSettings>

namespace Autotest {
namespace Internal {

static const char group[] = "Autotest";
static const char timeoutKey[] = "Timeout";
static const char metricsKey[] = "Metrics";
static const char omitInternalKey[] = "OmitInternal";
static const char omitRunConfigWarnKey[] = "OmitRCWarnings";
static const char limitResultOutputKey[] = "LimitResultOutput";
static const char autoScrollKey[] = "AutoScrollResults";
static const char alwaysParseKey[] = "AlwaysParse";
static const int defaultTimeout = 60000;

TestSettings::TestSettings()
    : timeout(defaultTimeout), metrics(Walltime), omitInternalMssg(true), omitRunConfigWarn(false),
      limitResultOutput(true), autoScroll(true), alwaysParse(false)
{
}

void TestSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(group));
    s->setValue(QLatin1String(timeoutKey), timeout);
    s->setValue(QLatin1String(metricsKey), metrics);
    s->setValue(QLatin1String(omitInternalKey), omitInternalMssg);
    s->setValue(QLatin1String(omitRunConfigWarnKey), omitRunConfigWarn);
    s->setValue(QLatin1String(limitResultOutputKey), limitResultOutput);
    s->setValue(QLatin1String(autoScrollKey), autoScroll);
    s->setValue(QLatin1String(alwaysParseKey), alwaysParse);
    s->endGroup();
}

static MetricsType intToMetrics(int value)
{
    switch (value) {
    case Walltime:
        return Walltime;
    case TickCounter:
        return TickCounter;
    case EventCounter:
        return EventCounter;
    case CallGrind:
        return CallGrind;
    case Perf:
        return Perf;
    default:
        return Walltime;
    }
}

void TestSettings::fromSettings(const QSettings *s)
{
    const QString root = QLatin1String(group) + QLatin1Char('/');
    timeout = s->value(root + QLatin1String(timeoutKey), defaultTimeout).toInt();
    metrics = intToMetrics(s->value(root + QLatin1String(metricsKey), Walltime).toInt());
    omitInternalMssg = s->value(root + QLatin1String(omitInternalKey), true).toBool();
    omitRunConfigWarn = s->value(root + QLatin1String(omitRunConfigWarnKey), false).toBool();
    limitResultOutput = s->value(root + QLatin1String(limitResultOutputKey), true).toBool();
    autoScroll = s->value(root + QLatin1String(autoScrollKey), true).toBool();
    alwaysParse = s->value(root + QLatin1String(alwaysParseKey), false).toBool();
}

bool TestSettings::equals(const TestSettings &rhs) const
{
    return timeout == rhs.timeout && metrics == rhs.metrics
            && omitInternalMssg == rhs.omitInternalMssg
            && omitRunConfigWarn == rhs.omitRunConfigWarn
            && limitResultOutput == rhs.limitResultOutput
            && autoScroll == rhs.autoScroll
            && alwaysParse == rhs.alwaysParse;
}

QString TestSettings::metricsTypeToOption(const MetricsType type)
{
    switch (type) {
    case MetricsType::Walltime:
        return QString();
    case MetricsType::TickCounter:
        return QLatin1String("-tickcounter");
    case MetricsType::EventCounter:
        return QLatin1String("-eventcounter");
    case MetricsType::CallGrind:
        return QLatin1String("-callgrind");
    case MetricsType::Perf:
        return QLatin1String("-perf");
    default:
        return QString();
    }
}

} // namespace Internal
} // namespace Autotest
