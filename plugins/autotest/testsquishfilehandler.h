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

#ifndef TESTSQUISHFILEHANDLER_H
#define TESTSQUISHFILEHANDLER_H

#include "testtreeitem.h"
#include "testtreemodel.h"

#include <QObject>
#include <QString>
#include <QMap>

namespace Autotest {
namespace Internal {

class TestSquishFileHandler : public QObject
{
    Q_OBJECT
public:
    explicit TestSquishFileHandler(QObject *parent = 0);
    static TestSquishFileHandler *instance();

    void openTestSuites();
    void closeTestSuite(const QString &suite);
    void closeAllTestSuites();

signals:
    void testTreeItemCreated(const TestTreeItem &item, TestTreeModel::Type type);
    void testTreeItemModified(const TestTreeItem &item, TestTreeModel::Type type, const QString &file);
    void testTreeItemsRemoved(const QString &filePath, TestTreeModel::Type type);

private:
    QMap<QString, QString> m_suites;

};

} // namespace Internal
} // namespace Autotest

#endif // TESTSQUISHFILEHANDLER_H
