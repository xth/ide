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

#include "opensquishsuitesdialog.h"
#include "testsquishfilehandler.h"
#include "testsquishutils.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

namespace Autotest {
namespace Internal {

static TestSquishFileHandler *m_instance = 0;

TestSquishFileHandler::TestSquishFileHandler(QObject *parent) : QObject(parent)
{
    m_instance = this;
}

TestSquishFileHandler *TestSquishFileHandler::instance()
{
    if (!m_instance)
        m_instance = new TestSquishFileHandler;

    return m_instance;
}

TestTreeItem createTestTreeItem(const QString &name, const QString &filePath,
                                const QStringList &cases)
{
    TestTreeItem item(name, filePath, TestTreeItem::SQUISH_SUITE);
    foreach (const QString &testCase, cases) {
        TestTreeItem *child = new TestTreeItem(QFileInfo(testCase).dir().dirName(), testCase,
                                               TestTreeItem::SQUISH_TESTCASE, &item);
        item.appendChild(child);
    }
    return item;
}

void TestSquishFileHandler::openTestSuites()
{
    OpenSquishSuitesDialog dialog;
    dialog.exec();
    foreach (const QString &suite, dialog.chosenSuites()) {
        const QDir suiteDir(suite);
        const QStringList cases = TestSquishUtils::validTestCases(suite);
        const QFileInfo suiteConf(suiteDir, QLatin1String("suite.conf"));
        if (m_suites.contains(suiteDir.dirName())) {
            if (QMessageBox::question(0, tr("Suite Already Open"),
                                      tr("A test suite with the name \"%1\" is already open. "
                                         "The opened test suite will be closed and replaced "
                                         "with the new one.").arg(suiteDir.dirName()),
                                      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel)
                    == QMessageBox::Cancel) {
                continue;
            } else {
                TestTreeItem item = createTestTreeItem(suiteDir.dirName(),
                                                       suiteConf.absoluteFilePath(), cases);
                // TODO update file watcher
                m_suites.insert(suiteDir.dirName(), suiteConf.absoluteFilePath());
                emit testTreeItemModified(item, TestTreeModel::SquishTest,
                                      m_suites.value(suiteDir.dirName()));
            }
        } else {
            TestTreeItem item = createTestTreeItem(suiteDir.dirName(),
                                                   suiteConf.absoluteFilePath(), cases);
            // TODO add file watcher
            m_suites.insert(suiteDir.dirName(), suiteConf.absoluteFilePath());
            emit testTreeItemCreated(item, TestTreeModel::SquishTest);
        }
    }
}

} // namespace Internal
} // namespace Autotest
