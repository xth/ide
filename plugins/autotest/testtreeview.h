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

#ifndef TESTTREEVIEW_H
#define TESTTREEVIEW_H

#include <utils/navigationtreeview.h>

#include <QModelIndex>

namespace Core {
class IContext;
}

namespace Autotest {
namespace Internal {

class TestTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT

public:
    TestTreeView(QWidget *parent = 0);

    void selectAll();
    void deselectAll();
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void runTestSuite(const QString &suiteName);
    void runTestCase(const QString &suiteName, const QString &testCaseName);
    void openObjectsMap(const QString &suiteName);
    void recordTestCase(const QString &suiteName, const QString &testCaseName);

private:
    void changeCheckStateAll(const Qt::CheckState checkState);
    Core::IContext *m_context;
    QModelIndex m_lastMousePressedIndex;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTTREEVIEW_H
