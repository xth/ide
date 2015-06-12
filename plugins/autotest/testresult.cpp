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

#include "testresult.h"

#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

FaultyTestResult::FaultyTestResult(Result::Type result, const QString &description)
    : TestResult(QString(), QString(), QString(), result, description)
{
}

TestResult::TestResult(const QString &className, const QString &testCase, const QString &dataTag,
                       Result::Type result, const QString &description)
    : m_class(className),
      m_case(testCase),
      m_dataTag(dataTag),
      m_result(result),
      m_description(description),
      m_line(0)
{
}

Result::Type TestResult::resultFromString(const QString &resultString)
{
    if (resultString == QLatin1String("pass"))
        return Result::PASS;
    if (resultString == QLatin1String("fail"))
        return Result::FAIL;
    if (resultString == QLatin1String("xfail"))
        return Result::EXPECTED_FAIL;
    if (resultString == QLatin1String("xpass"))
        return Result::UNEXPECTED_PASS;
    if (resultString == QLatin1String("skip"))
        return Result::SKIP;
    if (resultString == QLatin1String("qdebug"))
        return Result::MESSAGE_DEBUG;
    if (resultString == QLatin1String("warn") || resultString == QLatin1String("qwarn"))
        return Result::MESSAGE_WARN;
    if (resultString == QLatin1String("qfatal"))
        return Result::MESSAGE_FATAL;
    if (resultString == QLatin1String("bpass"))
        return Result::BLACKLISTED_PASS;
    if (resultString == QLatin1String("bfail"))
        return Result::BLACKLISTED_FAIL;
    qDebug("Unexpected test result: %s", qPrintable(resultString));
    return Result::UNKNOWN;
}

Result::Type TestResult::toResultType(int rt)
{
    switch(rt) {
    case Result::PASS:
        return Result::PASS;
    case Result::FAIL:
        return Result::FAIL;
    case Result::EXPECTED_FAIL:
        return Result::EXPECTED_FAIL;
    case Result::UNEXPECTED_PASS:
        return Result::UNEXPECTED_PASS;
    case Result::SKIP:
        return Result::SKIP;
    case Result::BLACKLISTED_PASS:
        return Result::BLACKLISTED_PASS;
    case Result::BLACKLISTED_FAIL:
        return Result::BLACKLISTED_FAIL;
    case Result::BENCHMARK:
        return Result::BENCHMARK;
    case Result::MESSAGE_DEBUG:
        return Result::MESSAGE_DEBUG;
    case Result::MESSAGE_WARN:
        return Result::MESSAGE_WARN;
    case Result::MESSAGE_FATAL:
        return Result::MESSAGE_FATAL;
    case Result::MESSAGE_INTERNAL:
        return Result::MESSAGE_INTERNAL;
    case Result::MESSAGE_CURRENT_TEST:
        return Result::MESSAGE_CURRENT_TEST;
    case Result::SQUISH_LOG:
        return Result::SQUISH_LOG;
    case Result::SQUISH_PASS:
        return Result::SQUISH_PASS;
    case Result::SQUISH_FAIL:
        return Result::SQUISH_FAIL;
    case Result::SQUISH_EXPECTED_FAIL:
        return Result::SQUISH_EXPECTED_FAIL;
    case Result::SQUISH_UNEXPECTED_PASS:
        return Result::SQUISH_UNEXPECTED_PASS;
    case Result::SQUISH_WARN:
        return Result::SQUISH_WARN;
    case Result::SQUISH_FATAL:
        return Result::SQUISH_FATAL;
    case Result::SQUISH_START:
        return Result::SQUISH_START;
    default:
        return Result::UNKNOWN;
    }
}

QString TestResult::resultToString(const Result::Type type)
{
    switch(type) {
    case Result::PASS:
        return QLatin1String("PASS");
    case Result::FAIL:
        return QLatin1String("FAIL");
    case Result::EXPECTED_FAIL:
        return QLatin1String("XFAIL");
    case Result::UNEXPECTED_PASS:
        return QLatin1String("XPASS");
    case Result::SKIP:
        return QLatin1String("SKIP");
    case Result::BENCHMARK:
        return QLatin1String("BENCH");
    case Result::MESSAGE_DEBUG:
        return QLatin1String("DEBUG");
    case Result::MESSAGE_WARN:
        return QLatin1String("WARN");
    case Result::MESSAGE_FATAL:
        return QLatin1String("FATAL");
    case Result::MESSAGE_INTERNAL:
    case Result::MESSAGE_CURRENT_TEST:
        return QString();
    case Result::BLACKLISTED_PASS:
        return QLatin1String("BPASS");
    case Result::BLACKLISTED_FAIL:
        return QLatin1String("BFAIL");
    case Result::SQUISH_LOG:
        return QLatin1String("Log");
    case Result::SQUISH_PASS:
        return QLatin1String("Pass");
    case Result::SQUISH_FAIL:
        return QLatin1String("Fail");
    case Result::SQUISH_ERROR:
        return QLatin1String("Error");
    case Result::SQUISH_FATAL:
        return QLatin1String("Fatal");
    case Result::SQUISH_EXPECTED_FAIL:
        return QLatin1String("Expected Fail");
    case Result::SQUISH_UNEXPECTED_PASS:
        return QLatin1String("Unexpected Pass");
    case Result::SQUISH_WARN:
        return QLatin1String("Warning");
    case Result::SQUISH_START:
        return QLatin1String("Start");
    case Result::SQUISH_END:
        return QLatin1String("Test finished");
    default:
        return QLatin1String("UNKNOWN");
    }
}

QColor TestResult::colorForType(const Result::Type type)
{
    switch(type) {
    case Result::PASS:
        return QColor("#009900");
    case Result::FAIL:
        return QColor("#a00000");
    case Result::EXPECTED_FAIL:
        return QColor("#00ff00");
    case Result::UNEXPECTED_PASS:
        return QColor("#ff0000");
    case Result::SKIP:
        return QColor("#787878");
    case Result::BLACKLISTED_PASS:
        return QColor(0, 0, 0);
    case Result::BLACKLISTED_FAIL:
        return QColor(0, 0, 0);
    case Result::MESSAGE_DEBUG:
        return QColor("#329696");
    case Result::MESSAGE_WARN:
        return QColor("#d0bb00");
    case Result::MESSAGE_FATAL:
        return QColor("#640000");
    case Result::MESSAGE_INTERNAL:
    case Result::MESSAGE_CURRENT_TEST:
        return QColor("transparent");
    case Result::SQUISH_LOG:
    case Result::SQUISH_START:
    case Result::SQUISH_END:
        return QColor(0, 0, 0);
    case Result::SQUISH_PASS:
        return QColor(0, 0x99, 0);
    case Result::SQUISH_FAIL:
    case Result::SQUISH_ERROR:
        return QColor(0xa0, 0, 0);
    case Result::SQUISH_EXPECTED_FAIL:
        return QColor(0, 0xff, 0);
    case Result::SQUISH_UNEXPECTED_PASS:
        return QColor(0xff, 0, 0);
    case Result::SQUISH_WARN:
        return QColor(0x86, 0, 0x86);
    case Result::SQUISH_FATAL:
        return QColor(0x64, 0, 0);
    default:
        return QColor(0, 0, 0);
    }
}

QString TestResult::maxString(const Result::Type type)
{
    if ((type >= Result::QTEST_GROUP_BEGIN && type <= Result::QTEST_GROUP_END)
            || type == Result::UNKNOWN) {
        return QLatin1String("UNKNOWN");
    } else if (type >= Result::SQUISH_GROUP_BEGIN && type <= Result::SQUISH_GROUP_END) {
        return QLatin1String("Unexpected Pass");
    }
    QTC_ASSERT(false, return QString());
}

bool operator==(const TestResult &t1, const TestResult &t2)
{
    return t1.className() == t2.className()
            && t1.testCase() == t2.testCase()
            && t1.dataTag() == t2.dataTag()
            && t1.result() == t2.result();
}

} // namespace Internal
} // namespace Autotest
