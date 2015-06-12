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

#ifndef TESTRESULTMODEL_H
#define TESTRESULTMODEL_H

#include "testresult.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QFont>
#include <QSet>

namespace Autotest {
namespace Internal {

namespace Result {
    enum ItemRole {
        TypeRole = Qt::UserRole
    };
} // namespace Result

class TestResultModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TestResultModel(QObject *parent = 0);
    ~TestResultModel();
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void addTestResult(const TestResult &testResult);
    void removeCurrentTestMessage();
    void clearTestResults();

    bool hasResults() const { return m_testResults.size() > 0; }
    TestResult testResult(const QModelIndex &index) const;

    int maxWidthOfFileName(const QFont &font);
    int maxWidthOfLineNumber(const QFont &font);

    int resultTypeCount(Result::Type type);

signals:

public slots:

private:
    QList<TestResult> m_testResults;
    QMap<Result::Type, int> m_testResultCount;
    int m_widthOfLineNumber;
    int m_maxWidthOfFileName;
    int m_lastMaxWidthIndex;
    QFont m_measurementFont;
};

class TestResultFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    TestResultFilterModel(TestResultModel *sourceModel, QObject *parent = 0);

    void enableAllResultTypes();
    void toggleTestResultType(Result::Type type);
    void clearTestResults();
    bool hasResults();
    TestResult testResult(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    TestResultModel *m_sourceModel;
    QSet<Result::Type> m_enabled;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTRESULTMODEL_H
