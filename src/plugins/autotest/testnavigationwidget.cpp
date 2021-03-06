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

#include "testnavigationwidget.h"
#include "testtreemodel.h"
#include "testtreeview.h"
#include "testtreeitemdelegate.h"
#include "testcodeparser.h"
#include "testrunner.h"
#include "autotestconstants.h"
#include "autotesticons.h"
#include "testtreeitem.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <texteditor/texteditor.h>
#include <utils/progressindicator.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace Autotest {
namespace Internal {

TestNavigationWidget::TestNavigationWidget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle(tr("Tests"));
    m_model = TestTreeModel::instance();
    m_sortFilterModel = new TestTreeSortFilterModel(m_model, m_model);
    m_sortFilterModel->setDynamicSortFilter(true);
    m_view = new TestTreeView(this);
    m_view->setModel(m_sortFilterModel);
    m_view->setSortingEnabled(true);
    m_view->setItemDelegate(new TestTreeItemDelegate(this));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_view));
    setLayout(layout);

    connect(m_view, &TestTreeView::activated, this, &TestNavigationWidget::onItemActivated);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicator::Medium, this);
    m_progressIndicator->attachToWidget(m_view);
    m_progressIndicator->hide();

    m_progressTimer = new QTimer(this);
    m_progressTimer->setSingleShot(true);
    m_progressTimer->setInterval(100); // don't display indicator if progress takes less than 100ms

    connect(m_model->parser(), &TestCodeParser::parsingStarted,
            this, &TestNavigationWidget::onParsingStarted);
    connect(m_model->parser(), &TestCodeParser::parsingFinished,
            this, &TestNavigationWidget::onParsingFinished);
    connect(m_model->parser(), &TestCodeParser::parsingFailed,
            this, &TestNavigationWidget::onParsingFinished);
    connect(m_progressTimer, &QTimer::timeout,
            m_progressIndicator, &Utils::ProgressIndicator::show);
}

TestNavigationWidget::~TestNavigationWidget()
{
    m_model->disableParsing();
}

void TestNavigationWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const bool enabled = !TestRunner::instance()->isTestRunning()
            && m_model->parser()->state() == TestCodeParser::Idle;
    const bool hasTests = m_model->hasTests();

    QMenu menu;
    QAction *runThisTest = 0;
    const QModelIndexList list = m_view->selectionModel()->selectedIndexes();
    if (list.size() == 1) {
        const QModelIndex index = list.first();
        QRect rect(m_view->visualRect(index));
        if (rect.contains(event->pos())) {
            // do not provide this menu entry for unnamed Quick Tests as it makes no sense
            int type = index.data(TypeRole).toInt();
            const QString &unnamed = tr(Constants::UNNAMED_QUICKTESTS);
            if ((type == TestTreeItem::TestFunction && index.parent().data().toString() != unnamed)
                    || (type == TestTreeItem::TestClass && index.data().toString() != unnamed)
                    || (type == TestTreeItem::TestDataTag)
                    || (type == TestTreeItem::GTestCase)
                    || (type == TestTreeItem::GTestCaseParameterized)
                    || (type == TestTreeItem::GTestName)) {
                runThisTest = new QAction(tr("Run This Test"), &menu);
                runThisTest->setEnabled(enabled);
                connect(runThisTest, &QAction::triggered,
                        this, &TestNavigationWidget::onRunThisTestTriggered);
            }
        }
    }

    QAction *runAll = Core::ActionManager::command(Constants::ACTION_RUN_ALL_ID)->action();
    QAction *runSelected = Core::ActionManager::command(Constants::ACTION_RUN_SELECTED_ID)->action();
    QAction *selectAll = new QAction(tr("Select All"), &menu);
    QAction *deselectAll = new QAction(tr("Deselect All"), &menu);
    // TODO remove?
    QAction *rescan = Core::ActionManager::command(Constants::ACTION_SCAN_ID)->action();

    connect(selectAll, &QAction::triggered, m_view, &TestTreeView::selectAll);
    connect(deselectAll, &QAction::triggered, m_view, &TestTreeView::deselectAll);

    runAll->setEnabled(enabled && hasTests);
    runSelected->setEnabled(enabled && hasTests);
    selectAll->setEnabled(enabled && hasTests);
    deselectAll->setEnabled(enabled && hasTests);
    rescan->setEnabled(enabled);

    if (runThisTest) {
        menu.addAction(runThisTest);
        menu.addSeparator();
    }
    menu.addAction(runAll);
    menu.addAction(runSelected);
    menu.addSeparator();
    menu.addAction(selectAll);
    menu.addAction(deselectAll);
    menu.addSeparator();
    menu.addAction(rescan);

    menu.exec(mapToGlobal(event->pos()));
}

QList<QToolButton *> TestNavigationWidget::createToolButtons()
{
    QList<QToolButton *> list;

    m_filterButton = new QToolButton(m_view);
    m_filterButton->setIcon(Core::Icons::FILTER.icon());
    m_filterButton->setToolTip(tr("Filter Test Tree"));
    m_filterButton->setProperty("noArrow", true);
    m_filterButton->setAutoRaise(true);
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterMenu = new QMenu(m_filterButton);
    initializeFilterMenu();
    connect(m_filterMenu, &QMenu::triggered, this, &TestNavigationWidget::onFilterMenuTriggered);
    m_filterButton->setMenu(m_filterMenu);

    m_sortAlphabetically = true;
    m_sort = new QToolButton(this);
    m_sort->setIcon(Icons::SORT_NATURALLY.icon());
    m_sort->setToolTip(tr("Sort Naturally"));

    QToolButton *expand = new QToolButton(this);
    expand->setIcon(Icons::EXPAND.icon());
    expand->setToolTip(tr("Expand All"));

    QToolButton *collapse = new QToolButton(this);
    collapse->setIcon(Icons::COLLAPSE.icon());
    collapse->setToolTip(tr("Collapse All"));

    connect(expand, &QToolButton::clicked, m_view, &TestTreeView::expandAll);
    connect(collapse, &QToolButton::clicked, m_view, &TestTreeView::collapseAll);
    connect(m_sort, &QToolButton::clicked, this, &TestNavigationWidget::onSortClicked);

    list << m_filterButton << m_sort << expand << collapse;
    return list;
}

void TestNavigationWidget::onItemActivated(const QModelIndex &index)
{
    const TextEditor::TextEditorWidget::Link link
            = index.data(LinkRole).value<TextEditor::TextEditorWidget::Link>();
    if (link.hasValidTarget()) {
        Core::EditorManager::openEditorAt(link.targetFileName, link.targetLine,
            link.targetColumn);
    }
}

void TestNavigationWidget::onSortClicked()
{
    if (m_sortAlphabetically) {
        m_sort->setIcon(Icons::SORT_ALPHABETICALLY.icon());
        m_sort->setToolTip(tr("Sort Alphabetically"));
        m_sortFilterModel->setSortMode(TestTreeSortFilterModel::Naturally);
    } else {
        m_sort->setIcon(Icons::SORT_NATURALLY.icon());
        m_sort->setToolTip(tr("Sort Naturally"));
        m_sortFilterModel->setSortMode(TestTreeSortFilterModel::Alphabetically);
    }
    m_sortAlphabetically = !m_sortAlphabetically;
}

void TestNavigationWidget::onFilterMenuTriggered(QAction *action)
{
    m_sortFilterModel->toggleFilter(
        TestTreeSortFilterModel::toFilterMode(action->data().value<int>()));
}

void TestNavigationWidget::onParsingStarted()
{
    m_progressTimer->start();
}

void TestNavigationWidget::onParsingFinished()
{
    m_progressTimer->stop();
    m_progressIndicator->hide();
}

void TestNavigationWidget::initializeFilterMenu()
{
    QAction *action = new QAction(m_filterMenu);
    action->setText(tr("Show Init and Cleanup Functions"));
    action->setCheckable(true);
    action->setChecked(false);
    action->setData(TestTreeSortFilterModel::ShowInitAndCleanup);
    m_filterMenu->addAction(action);
    action = new QAction(m_filterMenu);
    action->setText(tr("Show Data Functions"));
    action->setCheckable(true);
    action->setChecked(false);
    action->setData(TestTreeSortFilterModel::ShowTestData);
    m_filterMenu->addAction(action);
}

void TestNavigationWidget::onRunThisTestTriggered()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    // paranoia
    if (selected.isEmpty())
        return;
    const QModelIndex sourceIndex = m_sortFilterModel->mapToSource(selected.first());
    if (!sourceIndex.isValid())
        return;

    TestTreeItem *item = static_cast<TestTreeItem *>(sourceIndex.internalPointer());
    if (item->type() == TestTreeItem::TestClass || item->type() == TestTreeItem::TestFunction
            || item->type() == TestTreeItem::TestDataTag
            || item->type() == TestTreeItem::GTestCase
            || item->type() == TestTreeItem::GTestCaseParameterized
            || item->type() == TestTreeItem::GTestName) {
        if (TestConfiguration *configuration = m_model->getTestConfiguration(item)) {
            TestRunner *runner = TestRunner::instance();
            runner->setSelectedTests( {configuration} );
            runner->prepareToRunTests();
        }
    }
}

TestNavigationWidgetFactory::TestNavigationWidgetFactory()
{
    setDisplayName(tr("Tests"));
    setId(Autotest::Constants::AUTOTEST_ID);
    setPriority(666);
}

Core::NavigationView TestNavigationWidgetFactory::createWidget()
{
    TestNavigationWidget *treeViewWidget = new TestNavigationWidget;
    Core::NavigationView view;
    view.widget = treeViewWidget;
    view.dockToolBarWidgets = treeViewWidget->createToolButtons();
    TestTreeModel::instance()->enableParsing();
    return view;
}

} // namespace Internal
} // namespace Autotest
