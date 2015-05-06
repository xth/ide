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

#include "autotestconstants.h"
#include "testcodeparser.h"
#include "testrunner.h"
#include "testtreeitem.h"
#include "testtreeitemdelegate.h"
#include "testtreemodel.h"
#include "testtreeview.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <coreplugin/find/itemviewfind.h>

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <qmljstools/qmljsmodelmanager.h>

#include <texteditor/texteditor.h>

#include <QToolButton>
#include <QVBoxLayout>

namespace Autotest {
namespace Internal {

TestTreeView::TestTreeView(QWidget *parent)
    : NavigationTreeView(parent),
      m_context(new Core::IContext(this))
{
    setExpandsOnDoubleClick(false);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::AUTOTEST_CONTEXT));
    Core::ICore::addContextObject(m_context);
}

void TestTreeView::selectAll()
{
    changeCheckStateAll(Qt::Checked);
}

void TestTreeView::deselectAll()
{
    changeCheckStateAll(Qt::Unchecked);
}

// this avoids the re-evaluation of parent nodes when modifying the child nodes (setData())
void TestTreeView::changeCheckStateAll(const Qt::CheckState checkState)
{
    const TestTreeModel *model = TestTreeModel::instance();

    // 2 == Auto Tests and Quick Tests - must be raised if there will be others
    for (int rootRow = 0; rootRow < 2; ++rootRow) {
        QModelIndex currentRootIndex = model->index(rootRow, 0, rootIndex());
        if (!currentRootIndex.isValid())
            return;
        int count = model->rowCount(currentRootIndex);
        QModelIndex last;
        for (int classesRow = 0; classesRow < count; ++classesRow) {
            const QModelIndex classesIndex = model->index(classesRow, 0, currentRootIndex);
            int funcCount = model->rowCount(classesIndex);
            TestTreeItem *item = static_cast<TestTreeItem *>(classesIndex.internalPointer());
            if (item) {
                item->setChecked(checkState);
                if (!item->childCount())
                    last = classesIndex;
            }
            for (int functionRow = 0; functionRow < funcCount; ++functionRow) {
                last = model->index(functionRow, 0, classesIndex);
                TestTreeItem *item = static_cast<TestTreeItem *>(last.internalPointer());
                if (item)
                    item->setChecked(checkState);
            }
        }
        emit dataChanged(currentRootIndex, last);
    }
}

void TestTreeView::resizeEvent(QResizeEvent *event)
{
    // override derived behavior of Utils::NavigationTreeView as we might have more than 1 column
    TreeView::resizeEvent(event);
}

void TestTreeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid() && index.column() != 0) {
            auto type = TestTreeItem::toTestType(index.data(TypeRole).toInt());
            if (index.column() > 2)
                qWarning() << "Unexpected column:" << index.column();
            else if (type == TestTreeItem::SQUISH_SUITE || type == TestTreeItem::SQUISH_TESTCASE)
                m_lastMousePressedIndex = index;
        }
    }
    QTreeView::mousePressEvent(event);
}

void TestTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid() && m_lastMousePressedIndex == index) {
            auto type = TestTreeItem::toTestType(index.data(TypeRole).toInt());
            if (type == TestTreeItem::SQUISH_SUITE) {
                if (index.column() == 1)
                    emit runTestSuite(index.data().toString());
                else if (index.column() == 2)
                    emit openObjectsMap(index.data().toString());
            } else {
                const QModelIndex &parent = index.parent();
                if (parent.isValid()) {
                    if (index.column() == 1)
                        emit runTestCase(parent.data().toString(), index.data().toString());
                    else if (index.column() == 2)
                        emit recordTestCase(parent.data().toString(), index.data().toString());
                }
            }
        }
    }
    QTreeView::mouseReleaseEvent(event);
}

} // namespace Internal
} // namespace Autotest
