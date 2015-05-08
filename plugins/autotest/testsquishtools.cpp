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

#include "autotestplugin.h"
#include "squishsettings.h"
#include "testsquishtools.h"
#include "testresultspane.h"

#include <QDebug> // TODO remove

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QTimer>
#include <QWindow>

namespace Autotest {
namespace Internal {

// make this configurable?
static const QString resultsDirectory = QFileInfo(QDir::home(),
                                                  QLatin1String(".squishQC/Test Results")
                                                  ).absoluteFilePath();

TestSquishTools::TestSquishTools(QObject *parent)
    : QObject(parent),
      m_serverProcess(0),
      m_runnerProcess(0),
      m_serverPort(-1),
      m_request(None),
      m_state(Idle),
      m_currentResultsXML(0),
      m_resultsFileWatcher(0),
      m_testRunning(false)
{
    connect(this, &TestSquishTools::logOutputReceived,
            TestResultsPane::instance(), &TestResultsPane::addLogoutput, Qt::QueuedConnection);
}

TestSquishTools::~TestSquishTools()
{
    // TODO add confirmation dialog somewhere
    if (m_runnerProcess) {
        m_runnerProcess->terminate();
        if (!m_runnerProcess->waitForFinished(5000))
            m_runnerProcess->kill();
        delete m_runnerProcess;
        m_runnerProcess = 0;
    }

    if (m_serverProcess) {
        m_serverProcess->terminate();
        if (!m_serverProcess->waitForFinished(5000))
            m_serverProcess->kill();
        delete m_serverProcess;
        m_serverProcess = 0;
    }
}

struct SquishToolsSettings
{
    SquishToolsSettings()
        : serverPath(QLatin1String("squishserver"))
        , runnerPath(QLatin1String("squishrunner"))
        , isLocalServer(true)
        , verboseLog(false)
        , serverHost(QLatin1String("localhost"))
        , serverPort(9999)
    {}

    QString squishPath;
    QString serverPath;
    QString runnerPath;
    bool isLocalServer;
    bool verboseLog;
    QString serverHost;
    int serverPort;
    QString licenseKeyPath;
};

void TestSquishTools::runTestCases(const QString &suitePath, const QStringList &testCases,
                                   const QStringList &additionalServerArgs,
                                   const QStringList &additionalRunnerArgs)
{
    if (m_state != Idle) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Error"),
                              tr("Squish Tools in unexpected state (%1).\n"
                                 "Refusing to run a test case.").arg(m_state));
        return;
    }
    // create test results directory (if necessary) and return on fail
    if (!QDir().mkpath(resultsDirectory)) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Error"),
                              tr("Could not create test results folder. Canceling test run."));
        return;
    }

    m_suitePath = suitePath;
    m_testCases = testCases;
    m_reportFiles.clear();
    m_additionalServerArguments = additionalServerArgs;

    const QString dateTimeString
            = QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-ddTHH-mm-ss"));
    m_currentResultsDirectory = QFileInfo(QDir(resultsDirectory),
                                          dateTimeString).absoluteFilePath();

    m_additionalRunnerArguments = additionalRunnerArgs;
    m_additionalRunnerArguments << QLatin1String("--interactive")
                                << QLatin1String("--resultdir")
                                << QDir::toNativeSeparators(m_currentResultsDirectory);

    m_testRunning = true;
    emit squishTestRunStarted();
    startSquishServer(RunTestRequested);
}

void TestSquishTools::setState(TestSquishTools::State state)
{
    // TODO check whether state transition is legal
    m_state = state;

    switch (m_state) {
    case Idle:
        m_request = None;
        m_suitePath = QString();
        m_testCases.clear();
        m_reportFiles.clear();
        m_additionalRunnerArguments.clear();
        m_additionalServerArguments.clear();
        m_testRunning = false;
        m_currentResultsDirectory.clear();
        m_lastTopLevelWindows.clear();
        break;
    case ServerStarted:
        if (m_request == RunTestRequested) {
            startSquishRunner();
        } else if (m_request == RecordTestRequested) {

        } else if (m_request == RunnerQueryRequested) {

        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStartFailed:
        m_state = Idle;
        m_request = None;
        if (m_testRunning) {
            emit squishTestRunFinished();
            m_testRunning = false;
        }
        restoreQtCreatorWindows();
        break;
    case ServerStopped:
        m_state = Idle;
        if (m_request == ServerStopRequested) {
            m_request = None;
            if (m_testRunning) {
                emit squishTestRunFinished();
                m_testRunning = false;
            }
            restoreQtCreatorWindows();
        } else if (m_request == KillOldBeforeRunRunner) {
            startSquishServer(RunTestRequested);
        } else if (m_request == KillOldBeforeRecordRunner) {
            startSquishServer(RecordTestRequested);
        } else if (m_request == KillOldBeforeQueryRunner) {
            startSquishServer(RunnerQueryRequested);
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStopFailed:
        if (m_serverProcess && m_serverProcess->state() != QProcess::NotRunning) {
            m_serverProcess->terminate();
            if (!m_serverProcess->waitForFinished(5000)) {
                m_serverProcess->kill();
                delete m_serverProcess;
                m_serverProcess = 0;
            }
        }
        m_state = Idle;
        break;
    case RunnerStartFailed:
    case RunnerStopped:
        if (m_testCases.isEmpty()) {
            m_request = ServerStopRequested;
            stopSquishServer();
            // TODO merge result files
            logrotateTestResults();
        } else {
            startSquishRunner();
        }
        break;
    default:
        break;
    }
}

// will be populated by calling setupSquishTools() with current settings
static SquishToolsSettings toolsSettings;

void setupSquishTools()
{
    QSharedPointer<SquishSettings> squishSettings = AutotestPlugin::instance()->squishSettings();
    toolsSettings.squishPath = squishSettings->squishPath.toString();

    toolsSettings.serverPath
            = Utils::HostOsInfo::withExecutableSuffix(QLatin1String("squishserver"));
    toolsSettings.runnerPath
            = Utils::HostOsInfo::withExecutableSuffix(QLatin1String("squishrunner"));

    if (!toolsSettings.squishPath.isEmpty()) {
        const QDir squishBin(toolsSettings.squishPath + QDir::separator() + QLatin1String("bin"));
        toolsSettings.serverPath = QFileInfo(squishBin,
                                             toolsSettings.serverPath).absoluteFilePath();
        toolsSettings.runnerPath = QFileInfo(squishBin,
                                             toolsSettings.runnerPath).absoluteFilePath();
    }

    toolsSettings.isLocalServer = squishSettings->local;
    toolsSettings.serverHost = squishSettings->serverHost;
    toolsSettings.serverPort = squishSettings->serverPort;
    toolsSettings.verboseLog = squishSettings->verbose;
    toolsSettings.licenseKeyPath = squishSettings->licensePath.toString();
}

void TestSquishTools::startSquishServer(Request request)
{
    m_request = request;
    if (m_serverProcess) {
        if (QMessageBox::question(Core::ICore::dialogParent(), tr("Squish Server Already Running"),
                              tr("There is still an old Squish server instance running.\n"
                                 "This will cause problems later on.\n\n"
                                 "If you continue the old instance will be terminated.\n"
                                 "Do you want to continue?")) == QMessageBox::Yes) {
            switch (m_request) {
            case RunTestRequested:
                m_request = KillOldBeforeRunRunner;
                break;
            case RecordTestRequested:
                m_request = KillOldBeforeRecordRunner;
                break;
            case RunnerQueryRequested:
                m_request = KillOldBeforeQueryRunner;
                break;
            default:
                QMessageBox::critical(Core::ICore::dialogParent(), tr("Error"),
                                      tr("Unexpected state or request while starting Squish "
                                         "server. (state: %1, request: %2)")
                                      .arg(m_state).arg(m_request));
            }
            stopSquishServer();
        }
        return;
    }

    setupSquishTools();
    m_serverPort = -1;

    const Utils::FileName squishServer
            = Utils::Environment::systemEnvironment().searchInPath(toolsSettings.serverPath);
    if (squishServer.isEmpty()) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Squish Server Error"),
                              tr("\"%1\" could not be found or is not executable.\n"
                                 "Check the settings.")
                              .arg(QDir::toNativeSeparators(toolsSettings.serverPath)));
        setState(Idle);
        return;
    }
    toolsSettings.serverPath = squishServer.toString();

    if (true) // TODO squish setting of minimize QC on squish run/record
        minimizeQtCreatorWindows();
    else
        m_lastTopLevelWindows.clear();

    m_serverProcess = new QProcess;
    m_serverProcess->setProgram(toolsSettings.serverPath);
    QStringList arguments;
    // TODO if isLocalServer is false we should start a squishserver on remote device
    if (toolsSettings.isLocalServer)
        arguments << QLatin1String("--local"); // for now - although Squish Docs say "don't use it"
    else
        arguments << QLatin1String("--port") << QString::number(toolsSettings.serverPort);

    if (toolsSettings.verboseLog)
        arguments << QLatin1String("--verbose");

    m_serverProcess->setArguments(arguments);
    m_serverProcess->setProcessEnvironment(squishEnvironment());

    connect(m_serverProcess, &QProcess::readyReadStandardOutput,
            this, &TestSquishTools::onServerOutput);
    connect(m_serverProcess, &QProcess::readyReadStandardError,
            this, &TestSquishTools::onServerErrorOutput);
    connect(m_serverProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onServerFinished(int,QProcess::ExitStatus)));

    setState(ServerStarting);
    m_serverProcess->start();
    if (!m_serverProcess->waitForStarted()) {
        setState(ServerStartFailed);
        qWarning() << "squishserver did not start within 30s";
    }
}

void TestSquishTools::stopSquishServer()
{
    if (m_serverProcess && m_serverPort > 0) {
        QProcess serverKiller;
        serverKiller.setProgram(m_serverProcess->program());
        QStringList args;
        args << QLatin1String("--stop") << QLatin1String("--port") << QString::number(m_serverPort);
        serverKiller.setArguments(args);
        serverKiller.setProcessEnvironment(m_serverProcess->processEnvironment());
        serverKiller.start();
        if (serverKiller.waitForStarted()) {
            if (!serverKiller.waitForFinished()) {
                qWarning() << "Could not shutdown server within 30s";
                setState(ServerStopFailed);
            }
        } else {
            qWarning() << "Could not shutdown server within 30s";
            setState(ServerStopFailed);
        }
    } else {
        qWarning() << "either no process running or port < 1?" << m_serverProcess << m_serverPort;
        setState(ServerStopFailed);
    }
}

void TestSquishTools::startSquishRunner()
{
    if (!m_serverProcess) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("No Squish Server"),
                              tr("Squish server does not seem to be running.\n"
                                 "(state: %1, request: %2)\n"
                                 "Try again.").arg(m_state).arg(m_request));
        setState(Idle);
        return;
    }
    if (m_serverPort == -1) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("No Squish Server Port"),
                              tr("Failed to get the server port.\n"
                                 "(state: %1, request: %2)\n"
                                 "Try again.").arg(m_state).arg(m_request));
        // setting state to ServerStartFailed will terminate/kill the current unusable server
        setState(ServerStartFailed);
        return;
    }

    if (m_runnerProcess) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Squish Runner Running"),
                              tr("Squish runner seems to be running already.\n"
                                 "(state: %1, request: %2)\n"
                                 "Wait until it has finished and try again.")
                              .arg(m_state).arg(m_request));
        return;
    }

    const Utils::FileName squishRunner
            = Utils::Environment::systemEnvironment().searchInPath(toolsSettings.runnerPath);
    if (squishRunner.isEmpty()) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Squish Runner Error"),
                              tr("\"%1\" could not be found or is not executable.\n"
                                 "Check the settings.")
                              .arg(QDir::toNativeSeparators(toolsSettings.runnerPath)));
        setState(RunnerStopped);
        return;
    }
    toolsSettings.runnerPath = squishRunner.toString();

    m_runnerProcess = new QProcess;

    QStringList args;
    args << m_additionalServerArguments;
    if (!toolsSettings.isLocalServer)
        args << QLatin1String("--host") << toolsSettings.serverHost;
    args << QLatin1String("--port") << QString::number(m_serverPort);
    args << QLatin1String("--debugLog") << QLatin1String("alpw"); // TODO make this configurable?

    const QFileInfo testCasePath(QDir(m_suitePath), m_testCases.takeFirst());
    args << QLatin1String("--testcase") << testCasePath.absoluteFilePath();
    args << QLatin1String("--suitedir") << m_suitePath;

    args << m_additionalRunnerArguments;

    const QString caseReportFilePath = QFileInfo(QString::fromLatin1("%1/%2/%3/results.xml")
                                                 .arg(m_currentResultsDirectory,
                                                      QDir(m_suitePath).dirName(),
                                                      testCasePath.baseName())).absoluteFilePath();
    m_reportFiles.append(caseReportFilePath);

    args << QLatin1String("--reportgen")
         << QString::fromLatin1("xml2.2,%1").arg(caseReportFilePath);

    m_runnerProcess->setProgram(toolsSettings.runnerPath);
    m_runnerProcess->setArguments(args);
    m_runnerProcess->setProcessEnvironment(squishEnvironment());

    connect(m_runnerProcess, &QProcess::readyReadStandardError,
            this, &TestSquishTools::onRunnerErrorOutput);
    connect(m_runnerProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onRunnerFinished(int,QProcess::ExitStatus)));

    setState(RunnerStarting);

    // set up the file system watcher for being able to read the results.xml file
    m_resultsFileWatcher = new QFileSystemWatcher;
    // on second run this directory exists and won't emit changes, so use the current subdirectory
    if (QDir(m_currentResultsDirectory).exists())
        m_resultsFileWatcher->addPath(m_currentResultsDirectory + QDir::separator() + QDir(m_suitePath).dirName());
    else
        m_resultsFileWatcher->addPath(QFileInfo(m_currentResultsDirectory).absolutePath());

    connect(m_resultsFileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &TestSquishTools::onResultsDirChanged);

    m_runnerProcess->start();
    if (!m_runnerProcess->waitForStarted()) {
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Squish Runner Error"),
                              tr("Squish runner failed to start within given timeframe."));
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = 0;
        setState(RunnerStartFailed);
        return;
    }
    setState(RunnerStarted);
    m_currentResultsXML = new QFile(caseReportFilePath);
}

QProcessEnvironment TestSquishTools::squishEnvironment() const
{
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    if (!toolsSettings.licenseKeyPath.isEmpty())
        environment.prependOrSet(QLatin1String("SQUISH_LICENSEKEY_DIR"),
                                 toolsSettings.licenseKeyPath);
    environment.prependOrSet(QLatin1String("SQUISH_PREFIX"), toolsSettings.squishPath);
    return environment.toProcessEnvironment();
}

void TestSquishTools::onServerFinished(int, QProcess::ExitStatus)
{
    delete m_serverProcess;
    m_serverProcess = 0;
    m_serverPort = -1;
    setState(ServerStopped);
}

void TestSquishTools::onRunnerFinished(int, QProcess::ExitStatus)
{
    delete m_runnerProcess;
    m_runnerProcess = 0;

    if (m_resultsFileWatcher) {
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = 0;
    }
    if (m_currentResultsXML) {
        if (m_currentResultsXML->isOpen())
            m_currentResultsXML->close();
        delete m_currentResultsXML;
        m_currentResultsXML = 0;
    }
    setState(RunnerStopped);
}

void TestSquishTools::onServerOutput()
{
    // output used for getting the port information of the current squishserver
    const QByteArray output = m_serverProcess->readAllStandardOutput();
    foreach (const QByteArray &line, output.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        if (trimmed.startsWith("Port:")) {
            if (m_serverPort == -1) {
                bool ok;
                int port = trimmed.mid(6).toInt(&ok);
                if (ok) {
                    m_serverPort = port;
                    setState(ServerStarted);
                } else {
                    qWarning() << "could not get port number" << trimmed.mid(6);
                    setState(ServerStartFailed);
                }
            } else {
                qWarning() << "got a Port output - don't know why...";
            }
        }
        emit logOutputReceived(QLatin1String("Server: ") + QLatin1String(trimmed));
    }
}

void TestSquishTools::onServerErrorOutput()
{
    // output that must be send to the Runner/Server Log
    const QByteArray output = m_serverProcess->readAllStandardError();
    foreach (const QByteArray &line, output.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit logOutputReceived(QLatin1String("Server: ") + QLatin1String(trimmed));
    }
}

static char firstNonWhitespace(const QByteArray &text)
{
    for (int i = 0, limit = text.size(); i < limit; ++i)
        if (isspace(text.at(i)))
            continue;
        else
            return text.at(i);
    return 0;
}

static int positionAfterLastClosingTag(const QByteArray &text)
{
    QList<QByteArray> possibleEndTags;
    possibleEndTags << "</description>" << "</message>" << "</verification>" << "</result>"
                    << "</test>" << "</prolog>" << "</epilog>" << "</SquishReport>";

    int positionStart = text.lastIndexOf("</");
    if (positionStart == -1)
        return -1;

    int positionEnd = text.indexOf('>', positionStart);
    if (positionEnd == -1)
        return -1;

    QByteArray endTag = text.mid(positionStart, positionEnd + 1 - positionStart);
    if (possibleEndTags.contains(endTag))
        return positionEnd + 1;
    else
        return positionAfterLastClosingTag(text.mid(0, positionStart));
}

void TestSquishTools::onRunnerOutput(const QString)
{
    // buffer for already read, but not processed content
    static QByteArray buffer;
    const qint64 currentSize = m_currentResultsXML->size();

    if (currentSize <= m_readResultsCount)
        return;

    QByteArray output = m_currentResultsXML->read(currentSize - m_readResultsCount);
    if (output.isEmpty())
        return;

    if (!buffer.isEmpty())
        output.prepend(buffer);
    // we might read only partial written stuff - so we have to figure out how much we can
    // pass on for further processing and buffer the rest for the next reading
    const int endTag = positionAfterLastClosingTag(output);
    if (endTag < output.size()) {
        buffer = output.mid(endTag);
        output.truncate(endTag);
    } else {
        buffer.clear();
    }

    m_readResultsCount += output.size();

    if (firstNonWhitespace(output) == '<') {
        // output that must be used for the TestResultsPane
        qDebug() << "RunnerOutput:" << output;
    } else {
        foreach (const QByteArray &line, output.split('\n')) {
            const QByteArray trimmed = line.trimmed();
            if (!trimmed.isEmpty())
                emit logOutputReceived(QLatin1String("Runner: ") + QLatin1String(trimmed));
        }
    }
}

void TestSquishTools::onRunnerErrorOutput()
{
    // output that must be send to the Runner/Server Log
    const QByteArray output = m_runnerProcess->readAllStandardError();
    foreach (const QByteArray &line, output.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit logOutputReceived(QLatin1String("Runner: ") + QLatin1String(trimmed));
    }
}

void TestSquishTools::onResultsDirChanged(const QString &filePath)
{
    if (m_currentResultsXML->exists()) {
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = 0;
        m_readResultsCount = 0;
        if (m_currentResultsXML->open(QFile::ReadOnly)) {
            m_resultsFileWatcher = new QFileSystemWatcher;
            m_resultsFileWatcher->addPath(m_currentResultsXML->fileName());
            connect(m_resultsFileWatcher, &QFileSystemWatcher::fileChanged,
                    this, &TestSquishTools::onRunnerOutput);
        } else {
            // TODO set a flag to process results.xml as soon the complete test run has finished
            qWarning() << "could not open results.xml although it exists"
                       << filePath << m_currentResultsXML->error()
                       << m_currentResultsXML->errorString();
        }
    } else {
        disconnect(m_resultsFileWatcher);
        // results.xml is created as soon some output has been opened - so try again in a second
        QTimer::singleShot(1000, this, [this, filePath] () {
            onResultsDirChanged(filePath);
        });
    }
}

void TestSquishTools::logrotateTestResults()
{
    // make this configurable?
    const int maxNumberOfTestResults = 10;
    const QStringList existing = QDir(resultsDirectory).entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                                  QDir::Name);

    for (int i = 0, limit = existing.size() - maxNumberOfTestResults; i < limit; ++i) {
        QDir current(resultsDirectory + QDir::separator() + existing.at(i));
        if (!current.removeRecursively())
            qWarning() << "could not remove" << current.absolutePath();
    }
}

void TestSquishTools::minimizeQtCreatorWindows()
{
    m_lastTopLevelWindows = QApplication::topLevelWindows();
    QWindowList toBeRemoved;
    foreach (QWindow *window, m_lastTopLevelWindows) {
        if (window->isVisible())
            window->showMinimized();
        else
            toBeRemoved.append(window);
    }

    foreach (QWindow *window, toBeRemoved)
        m_lastTopLevelWindows.removeOne(window);
}

void TestSquishTools::restoreQtCreatorWindows()
{
    foreach (QWindow *window, m_lastTopLevelWindows) {
        window->requestActivate();
        window->showNormal();
    }
}

} // namespace Internal
} // namespace Autotest
