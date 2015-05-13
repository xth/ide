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

#ifndef TESTSQUISHTOOLS_H
#define TESTSQUISHTOOLS_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QWindowList>

QT_BEGIN_NAMESPACE
class QFile;
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

class SquishXmlOutputHandler;

class TestSquishTools : public QObject
{
    Q_OBJECT
public:
    explicit TestSquishTools(QObject *parent = 0);
    ~TestSquishTools();

    enum State
    {
        Idle,
        ServerStarting,
        ServerStarted,
        ServerStartFailed,
        ServerStopped,
        ServerStopFailed,
        RunnerStarting,
        RunnerStarted,
        RunnerStartFailed,
        RunnerStopped
    };

    State state() const { return m_state; }

    void runTestCases(const QString &suitePath, const QStringList &testCases = QStringList(),
                      const QStringList &additionalServerArgs = QStringList(),
                      const QStringList &additionalRunnerArgs = QStringList());

signals:
    void logOutputReceived(const QString &output);
    void squishTestRunStarted();
    void squishTestRunFinished();
    void resultOutputCreated(const QByteArray &output);

private:
    enum Request
    {
        None,
        ServerStopRequested,
        ServerQueryRequested,
        RunnerQueryRequested,
        RunTestRequested,
        RecordTestRequested,
        KillOldBeforeRunRunner,
        KillOldBeforeRecordRunner,
        KillOldBeforeQueryRunner
    };

    void setState(State state);

    void startSquishServer(Request request);
    void stopSquishServer();
    void startSquishRunner();
    QProcessEnvironment squishEnvironment() const;
    Q_SLOT void onServerFinished(int exitCode, QProcess::ExitStatus status = QProcess::NormalExit);
    Q_SLOT void onRunnerFinished(int exitCode, QProcess::ExitStatus status = QProcess::NormalExit);
    void onServerOutput();
    void onServerErrorOutput();
    void onRunnerOutput(const QString);
    void onRunnerErrorOutput();
    void onResultsDirChanged(const QString &filePath);
    void logrotateTestResults();
    void minimizeQtCreatorWindows();
    void restoreQtCreatorWindows();

    QProcess *m_serverProcess;
    QProcess *m_runnerProcess;
    int m_serverPort;
    QString m_serverHost;
    Request m_request;
    State m_state;
    QString m_suitePath;
    QStringList m_testCases;
    QStringList m_reportFiles;
    QString m_currentResultsDirectory;
    QFile *m_currentResultsXML;
    QFileSystemWatcher *m_resultsFileWatcher;
    QStringList m_additionalServerArguments;
    QStringList m_additionalRunnerArguments;
    QWindowList m_lastTopLevelWindows;
    bool m_testRunning;
    qint64 m_readResultsCount;
    SquishXmlOutputHandler *m_xmlOutputHandler;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTSQUISHTOOLS_H
