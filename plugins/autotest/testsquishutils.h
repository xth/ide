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

#ifndef TESTSQUISHUTILS_H
#define TESTSQUISHUTILS_H

#include <QString>
#include <QVariantMap>

namespace Autotest {
namespace Internal {

class TestSquishUtils
{
public:
    static QStringList validTestCases(QString baseDirectory);
    static QVariantMap readSuiteConf(QString suiteConfPath);
    static QString extensionForLanguage(QString language);

private:
    TestSquishUtils() {}
};

} // namespace Internal
} // namespace Autotest

#endif // TESTSQUISHUTILS_H
