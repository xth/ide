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

#ifndef SQUISHXMLOUTPUTHANDLER_H
#define SQUISHXMLOUTPUTHANDLER_H

#include "testresult.h"

#include <QObject>
#include <QXmlStreamReader>

namespace Autotest {
namespace Internal {

class SquishXmlOutputHandler : public QObject
{
    Q_OBJECT
public:
    explicit SquishXmlOutputHandler(QObject *parent = 0);
    void clearForNextRun();

    static void mergeResultFiles(const QStringList &reportFiles, const QString &resultsDirectory,
                                 const QString &suiteName, QString *errorMessage = 0);

signals:
    void testResultCreated(const TestResult &testResult);

public slots:
    void outputAvailable(const QByteArray &output);

private:
    QXmlStreamReader m_xmlReader;
};

} // namespace Internal
} // namespace Autotest

#endif // SQUISHXMLOUTPUTHANDLER_H
