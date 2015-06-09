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

#include "testnavigationwidget.h"
#include "testsquishfilehandler.h"
#include "testtreemodel.h"
#include "testtreeview.h"
#include "testtreeitemdelegate.h"
#include "testcodeparser.h"
#include "testrunner.h"
#include "autotestconstants.h"
#include "testtreeitem.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <texteditor/texteditor.h>
#include <utils/progressindicator.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace Autotest {
namespace Internal {

const int defaultSectionSize = 18;

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
    QHeaderView *header = new QHeaderView(Qt::Horizontal, m_view);
    header->setModel(m_model);
    header->setDefaultSectionSize(0);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    m_view->setHeader(header);
    m_view->setHeaderHidden(true);

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
    connect(m_progressTimer, &QTimer::timeout,
            m_progressIndicator, &Utils::ProgressIndicator::show);

    connect(m_view, &QTreeView::expanded, this, &TestNavigationWidget::onExpanded);
    connect(m_view, &QTreeView::collapsed, this, &TestNavigationWidget::onCollapsed);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &TestNavigationWidget::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &TestNavigationWidget::onRowsRemoved);

    connect(m_view, &TestTreeView::runTestCase,
            TestSquishFileHandler::instance(), &TestSquishFileHandler::runTestCase);
    connect(m_view, &TestTreeView::runTestSuite,
            TestSquishFileHandler::instance(), &TestSquishFileHandler::runTestSuite);
}

TestNavigationWidget::~TestNavigationWidget()
{
    m_model->disableParsing();
}

bool TestNavigationWidget::handleSquishContextMenuEvent(QContextMenuEvent *event, bool enabled)
{
    bool isSquishMenu = false;
    QMenu menu;

    // item specific menu entries
    const QModelIndexList list = m_view->selectionModel()->selectedIndexes();
    // 3 columns - but we only need the data of the first column as the others contain only an icon
    if (list.size() == 3) {
        const QModelIndex &index = list.first();
        QRect rect(m_view->visualRect(index));
        rect.adjust(0, 0, defaultSectionSize * 2, 0);
        if (rect.contains(event->pos())) {
            TestTreeItem::Type type = TestTreeItem::toTestType(index.data(TypeRole).toInt());

            if (type == TestTreeItem::SQUISH_TESTCASE) {
                const QString caseName = index.data().toString();
                const QString suiteName = index.parent().data().toString();
                isSquishMenu = true;
                QAction *runThisTestCase = new QAction(tr("Run This Test Case"), &menu);
                menu.addAction(runThisTestCase);
                runThisTestCase->setEnabled(enabled);
                QAction *deleteTestCase = new QAction(tr("Delete Test Case"), &menu);
                menu.addAction(deleteTestCase);
                deleteTestCase->setEnabled(enabled);
                menu.addSeparator();

                connect(runThisTestCase, &QAction::triggered, [suiteName, caseName] () {
                    TestSquishFileHandler::instance()->runTestCase(suiteName, caseName);
                });
            } else if (type == TestTreeItem::SQUISH_SUITE) {
                const QString suiteName = index.data().toString();
                isSquishMenu = true;
                QAction *runThisTestSuite = new QAction(tr("Run This Test Suite"), &menu);
                menu.addAction(runThisTestSuite);
                runThisTestSuite->setEnabled(enabled);
                menu.addSeparator();
                QAction *addNewTestCase = new QAction(tr("Add New Test Case..."), &menu);
                menu.addAction(addNewTestCase);
                addNewTestCase->setEnabled(enabled);
                QAction *closeTestSuite = new QAction(tr("Close Test Suite"), &menu);
                menu.addAction(closeTestSuite);
                closeTestSuite->setEnabled(enabled);
                QAction *deleteTestSuite = new QAction(tr("Delete Test Suite"), &menu);
                menu.addAction(deleteTestSuite);
                deleteTestSuite->setEnabled(enabled);
                menu.addSeparator();

                connect(runThisTestSuite, &QAction::triggered,
                        [suiteName] () {
                   TestSquishFileHandler::instance()->runTestSuite(suiteName);
                });
                connect(closeTestSuite, &QAction::triggered,
                        [suiteName] () {
                    TestSquishFileHandler::instance()->closeTestSuite(suiteName);
                });
            }
        }
    }
    // ROOT items aren't selectable - so, check for them different way
    QModelIndex squishIndex = m_view->model()->index(2, 0);
    QRect squishRootRect(m_view->visualRect(squishIndex));
    if (squishRootRect.contains(event->pos()))
        isSquishMenu = true;

    if (isSquishMenu) {
        // general squish related menu entries
        QAction *openSquishSuites = new QAction(tr("Open Squish Suites..."), &menu);
        menu.addAction(openSquishSuites);
        openSquishSuites->setEnabled(enabled);
        QAction *createNewTestSuite = new QAction(tr("Create New Test Suite..."), &menu);
        menu.addAction(createNewTestSuite);
        createNewTestSuite->setEnabled(enabled);

        connect(openSquishSuites, &QAction::triggered,
                TestSquishFileHandler::instance(), &TestSquishFileHandler::openTestSuites);

        if (m_view->model()->rowCount(squishIndex) > 0) {
            menu.addSeparator();
            QAction *closeAllSuites = new QAction(tr("Close All Test Suites"), &menu);
            menu.addAction(closeAllSuites);
            closeAllSuites->setEnabled(enabled);

            connect(closeAllSuites, &QAction::triggered, [this] () {
                if (QMessageBox::question(this, tr("Close All Test Suites"),
                                          tr("Are you sure you want to close all test suites?"
                                             /*"\nThis will close all related files as well."*/))
                        == QMessageBox::Yes)
                    TestSquishFileHandler::instance()->closeAllTestSuites();
            });
        }

        menu.exec(mapToGlobal(event->pos()));
    }
    return isSquishMenu;
}

void TestNavigationWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const bool enabled = !TestRunner::instance()->isTestRunning()
            && m_model->parser()->state() == TestCodeParser::Idle;
    const bool hasTests = m_model->hasTests();

    if (handleSquishContextMenuEvent(event, enabled))
        return;

    QMenu menu;
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
    m_filterButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
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
    m_sort->setIcon((QIcon(QLatin1String(":/images/leafsort.png"))));
    m_sort->setToolTip(tr("Sort Naturally"));

    QToolButton *expand = new QToolButton(this);
    expand->setIcon(QIcon(QLatin1String(":/images/expand.png")));
    expand->setToolTip(tr("Expand All"));

    QToolButton *collapse = new QToolButton(this);
    collapse->setIcon(QIcon(QLatin1String(":/images/collapse.png")));
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
        m_sort->setIcon((QIcon(QLatin1String(":/images/sort.png"))));
        m_sort->setToolTip(tr("Sort Alphabetically"));
        m_sortFilterModel->setSortMode(TestTreeSortFilterModel::Naturally);
    } else {
        m_sort->setIcon((QIcon(QLatin1String(":/images/leafsort.png"))));
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

void TestNavigationWidget::onExpanded(const QModelIndex &index)
{
    if (index.data().toString().startsWith(QLatin1String("Squish")))
        m_view->header()->setDefaultSectionSize(defaultSectionSize);
}

void TestNavigationWidget::onCollapsed(const QModelIndex &index)
{
    if (index.data().toString().startsWith(QLatin1String("Squish")))
        m_view->header()->setDefaultSectionSize(0);
}

void TestNavigationWidget::onRowsInserted(const QModelIndex &parent, int, int)
{
    if (parent.isValid() && parent.data().toString().startsWith(QLatin1String("Squish")))
        if (m_view->isExpanded(parent) && m_model->rowCount(parent))
            m_view->header()->setDefaultSectionSize(defaultSectionSize);
}

void TestNavigationWidget::onRowsRemoved(const QModelIndex &parent, int, int)
{
    if (parent.isValid() && parent.data().toString().startsWith(QLatin1String("Squish")))
        if (m_model->rowCount(parent) == 0)
            m_view->header()->setDefaultSectionSize(0);
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
