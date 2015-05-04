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

#include "testsquishutils.h"

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QSettings>

namespace Autotest {
namespace Internal {

const static QString squishLanguageKey = QLatin1String("LANGUAGE");
const static QString squishTestCasesKey = QLatin1String("TEST_CASES");

QStringList TestSquishUtils::validTestCases(QString baseDirectory)
{
    QStringList validCases;
    QDir subDir(baseDirectory);
    QFileInfo suiteConf(subDir, QLatin1String("suite.conf"));
    if (suiteConf.exists()) {
        QVariantMap conf = readSuiteConf(suiteConf.absoluteFilePath());
        QString extension = extensionForLanguage(conf.value(squishLanguageKey).toString());
        QStringList cases = conf.value(squishTestCasesKey).toString().split(
                    QRegExp(QLatin1String("\\s+")));

        foreach (const QString &testCase, cases) {
            QFileInfo testCaseDirInfo(subDir, testCase);
            if (testCaseDirInfo.isDir()) {
                QFileInfo testCaseTestInfo(testCaseDirInfo.filePath(),
                                           QLatin1String("test") + extension);
                if (testCaseTestInfo.isFile())
                    validCases.append(testCaseTestInfo.absoluteFilePath());
            }
        }
    }

    return validCases;
}

QVariantMap TestSquishUtils::readSuiteConf(QString suiteConfPath)
{
    const QSettings suiteConf(suiteConfPath, QSettings::IniFormat);
    QVariantMap result;
    // TODO get all information - actually only the information needed now is fetched
    result.insert(squishLanguageKey, suiteConf.value(squishLanguageKey));
    result.insert(squishTestCasesKey, suiteConf.value(squishTestCasesKey));
    return result;
}

QString TestSquishUtils::extensionForLanguage(QString language)
{
    if (language == QLatin1String("Python"))
        return QLatin1String(".py");
    else if (language == QLatin1String("Perl"))
        return QLatin1String(".pl");
    else if (language == QLatin1String("JavaScript"))
        return QLatin1String(".js");
    else if (language == QLatin1String("Ruby"))
        return QLatin1String(".rb");
    else if (language == QLatin1String("Tcl"))
        return QLatin1String(".tcl");
    else
        return QString(); // better return an invalid extension?
}

} // namespace Internal
} // namespace Autotest
