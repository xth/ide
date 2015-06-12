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

#include "testresultmodel.h"

#include <QDebug>
#include <QFontMetrics>
#include <QIcon>
#include <QSortFilterProxyModel>

namespace Autotest {
namespace Internal {

TestResultModel::TestResultModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_widthOfLineNumber(0),
    m_maxWidthOfFileName(0),
    m_lastMaxWidthIndex(0)
{
}

TestResultModel::~TestResultModel()
{
    m_testResults.clear();
}

QModelIndex TestResultModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex TestResultModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int TestResultModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_testResults.size();
}

int TestResultModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

static QIcon testResultIcon(Result::Type result) {
    static QIcon icons[11] = {
        QIcon(QLatin1String(":/images/pass.png")),
        QIcon(QLatin1String(":/images/fail.png")),
        QIcon(QLatin1String(":/images/xfail.png")),
        QIcon(QLatin1String(":/images/xpass.png")),
        QIcon(QLatin1String(":/images/skip.png")),
        QIcon(QLatin1String(":/images/blacklisted_pass.png")),
        QIcon(QLatin1String(":/images/blacklisted_fail.png")),
        QIcon(QLatin1String(":/images/benchmark.png")),
        QIcon(QLatin1String(":/images/debug.png")),
        QIcon(QLatin1String(":/images/warn.png")),
        QIcon(QLatin1String(":/images/fatal.png")),
    }; // provide an icon for unknown??

    if (result < 0 || result >= Result::MESSAGE_INTERNAL)
        return QIcon();
    return icons[result];
}

QVariant TestResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_testResults.count() || index.column() != 0)
        return QVariant();
    if (role == Qt::DisplayRole) {
        const TestResult &tr = m_testResults.at(index.row());
        switch (tr.result()) {
        case Result::PASS:
        case Result::FAIL:
        case Result::EXPECTED_FAIL:
        case Result::UNEXPECTED_PASS:
        case Result::SKIP:
        case Result::BLACKLISTED_PASS:
        case Result::BLACKLISTED_FAIL:
        case Result::BENCHMARK:
            return QString::fromLatin1("%1::%2 (%3) - %4").arg(tr.className(), tr.testCase(),
                                                               tr.dataTag(), tr.fileName());
        default:
            return tr.description();
        }
    }
    if (role == Qt::DecorationRole) {
        const TestResult &tr = m_testResults.at(index.row());
        return testResultIcon(tr.result());
    }
    if (role == Result::TypeRole) {
        return m_testResults.at(index.row()).result();
    }

    return QVariant();
}

void TestResultModel::addTestResult(const TestResult &testResult)
{
    const bool isCurrentTestMssg = testResult.result() == Result::MESSAGE_CURRENT_TEST;
    TestResult lastMssg = m_testResults.empty() ? TestResult() : m_testResults.last();

    int position = m_testResults.size();

    if (isCurrentTestMssg && lastMssg.result() == Result::MESSAGE_CURRENT_TEST) {
        lastMssg.setDescription(testResult.description());
        m_testResults.replace(m_testResults.size() - 1, lastMssg);
        const QModelIndex changed = index(m_testResults.size() - 1, 0, QModelIndex());
        emit dataChanged(changed, changed);
    } else {
        // decrement only if last message was of type MESSAGE_CURRENT_TEST
        if (!isCurrentTestMssg && lastMssg.result() == Result::MESSAGE_CURRENT_TEST)
            --position;
        beginInsertRows(QModelIndex(), position, position);
        m_testResults.insert(position, testResult);
        endInsertRows();
    }

    if (!isCurrentTestMssg) {
        int count = m_testResultCount.value(testResult.result(), 0);
        m_testResultCount.insert(testResult.result(), ++count);
    }
}

void TestResultModel::removeCurrentTestMessage()
{
    TestResult lastMssg = m_testResults.empty() ? TestResult() : m_testResults.last();
    if (lastMssg.result() == Result::MESSAGE_CURRENT_TEST) {
        beginRemoveRows(QModelIndex(), m_testResults.size() - 1, m_testResults.size() - 1);
        m_testResults.removeLast();
        endRemoveRows();
    }
}

void TestResultModel::clearTestResults()
{
    if (m_testResults.size() == 0)
        return;
    beginRemoveRows(QModelIndex(), 0, m_testResults.size() - 1);
    m_testResults.clear();
    m_testResultCount.clear();
    m_lastMaxWidthIndex = 0;
    m_maxWidthOfFileName = 0;
    m_widthOfLineNumber = 0;
    endRemoveRows();
}

TestResult TestResultModel::testResult(const QModelIndex &index) const
{
    if (!index.isValid())
        return TestResult(QString(), QString());
    return m_testResults.at(index.row());
}

int TestResultModel::maxWidthOfFileName(const QFont &font)
{
    int count = m_testResults.size();
    if (count == 0)
        return 0;
    if (m_maxWidthOfFileName > 0 && font == m_measurementFont && m_lastMaxWidthIndex == count - 1)
        return m_maxWidthOfFileName;

    QFontMetrics fm(font);
    m_measurementFont = font;

    for (int i = m_lastMaxWidthIndex; i < count; ++i) {
        QString filename = m_testResults.at(i).fileName();
        const int pos = filename.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            filename = filename.mid(pos +1);

        m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.width(filename));
    }
    m_lastMaxWidthIndex = count - 1;
    return m_maxWidthOfFileName;
}

int TestResultModel::maxWidthOfLineNumber(const QFont &font)
{
    if (m_widthOfLineNumber == 0 || font != m_measurementFont) {
        QFontMetrics fm(font);
        m_measurementFont = font;
        m_widthOfLineNumber = fm.width(QLatin1String("88888"));
    }
    return m_widthOfLineNumber;
}

int TestResultModel::resultTypeCount(Result::Type type)
{
    return m_testResultCount.value(type, 0);
}

/********************************** Filter Model **********************************/

TestResultFilterModel::TestResultFilterModel(TestResultModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sourceModel(sourceModel)
{
    setSourceModel(sourceModel);
    enableAllResultTypes();
}

void TestResultFilterModel::enableAllResultTypes()
{
    m_enabled << Result::PASS << Result::FAIL << Result::EXPECTED_FAIL
              << Result::UNEXPECTED_PASS << Result::SKIP << Result::MESSAGE_DEBUG
              << Result::MESSAGE_WARN << Result::MESSAGE_INTERNAL
              << Result::MESSAGE_FATAL << Result::UNKNOWN << Result::BLACKLISTED_PASS
              << Result::BLACKLISTED_FAIL << Result::BENCHMARK
              << Result::MESSAGE_CURRENT_TEST
              << Result::SQUISH_LOG << Result::SQUISH_PASS << Result::SQUISH_FAIL
              << Result::SQUISH_EXPECTED_FAIL << Result::SQUISH_UNEXPECTED_PASS
              << Result::SQUISH_WARN << Result::SQUISH_ERROR << Result::SQUISH_FATAL
              << Result::SQUISH_START << Result::SQUISH_END;
    invalidateFilter();
}

void TestResultFilterModel::toggleTestResultType(Result::Type type)
{
    if (m_enabled.contains(type)) {
        m_enabled.remove(type);
    } else {
        m_enabled.insert(type);
    }

    switch (type) {
    case Result::PASS:
        toggleTestResultType(Result::SQUISH_PASS);
        break;
    case Result::FAIL:
        toggleTestResultType(Result::SQUISH_FAIL);
        break;
    case Result::EXPECTED_FAIL:
        toggleTestResultType(Result::SQUISH_EXPECTED_FAIL);
        break;
    case Result::UNEXPECTED_PASS:
        toggleTestResultType(Result::SQUISH_UNEXPECTED_PASS);
        break;
    case Result::MESSAGE_WARN:
        toggleTestResultType(Result::SQUISH_WARN);
        break;
    default:
        invalidateFilter();
    }
}

void TestResultFilterModel::clearTestResults()
{
    m_sourceModel->clearTestResults();
}

bool TestResultFilterModel::hasResults()
{
    return rowCount(QModelIndex());
}

TestResult TestResultFilterModel::testResult(const QModelIndex &index) const
{
    return m_sourceModel->testResult(mapToSource(index));
}

bool TestResultFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = m_sourceModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;
    return m_enabled.contains(m_sourceModel->testResult(index).result());
}

} // namespace Internal
} // namespace Autotest
