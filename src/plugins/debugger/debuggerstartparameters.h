/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_DEBUGGERSTARTPARAMETERS_H
#define DEBUGGER_DEBUGGERSTARTPARAMETERS_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <coreplugin/id.h>
#include <ssh/sshconnection.h>
#include <utils/environment.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QMetaType>

namespace Debugger {

// Note: This is part of the "soft interface" of the debugger plugin.
// Do not add anything that needs implementation in a .cpp file.

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    enum CommunicationChannel {
        CommunicationChannelTcpIp,
        CommunicationChannelUsb
    };

    DebuggerStartParameters()
      : masterEngineType(NoEngineType),
        firstSlaveEngineType(NoEngineType),
        secondSlaveEngineType(NoEngineType),
        cppEngineType(NoEngineType),
        isSnapshot(false),
        attachPID(-1),
        useTerminal(false),
        breakOnMain(false),
        continueAfterAttach(false),
        multiProcess(false),
        languages(AnyLanguage),
        qmlServerAddress(QLatin1String("127.0.0.1")),
        qmlServerPort(Constants::QML_DEFAULT_DEBUG_SERVER_PORT),
        remoteSetupNeeded(false),
        useContinueInsteadOfRun(false),
        startMode(NoStartMode),
        closeMode(KillAtClose),
        useCtrlCStub(false),
        testReceiver(0),
        testCallback(0),
        testCase(0)
    {}

    //Core::Id profileId;

    DebuggerEngineType masterEngineType;
    DebuggerEngineType firstSlaveEngineType;
    DebuggerEngineType secondSlaveEngineType;
    DebuggerEngineType cppEngineType;
    QString sysRoot;
    QString deviceSymbolsRoot;
    QString debuggerCommand;
    ProjectExplorer::Abi toolChainAbi;
    ProjectExplorer::IDevice::ConstPtr device;

    QString platform;
    QString executable;
    QString displayName; // Used in the Snapshots view.
    QString startMessage; // First status message shown.
    QString coreFile;
    QString overrideStartScript; // Used in attach to core and remote debugging
    bool isSnapshot; // Set if created internally.
    QString processArgs;
    Utils::Environment environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;
    bool breakOnMain;
    bool continueAfterAttach;
    bool multiProcess;
    DebuggerLanguages languages;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    // Used by Qml debugging.
    QString qmlServerAddress;
    quint16 qmlServerPort;
    QString projectSourceDirectory;
    QString projectBuildDirectory;
    QStringList projectSourceFiles;

    // Used by remote debugging.
    QString remoteChannel;
    QString serverStartScript;
    QString debugInfoLocation; // Gdb "set-debug-file-directory".
    QStringList debugSourceLocation; // Gdb "directory"
    QByteArray remoteSourcesDir;
    QString remoteMountPoint;
    QString localMountDir;
    QSsh::SshConnectionParameters connParams;
    bool remoteSetupNeeded;
    QMap<QString, QString> sourcePathMap;

    // Used by baremetal plugin
    QByteArray commandsForReset; // commands used for resetting the inferior
    bool useContinueInsteadOfRun; // if connected to a hw debugger run is not possible but continue is used
    QByteArray commandsAfterConnect; // additional commands to post after connection to debug target

    QStringList solibSearchPath;
    DebuggerStartMode startMode;
    DebuggerCloseMode closeMode;

    // For QNX debugging
    QString remoteExecutable;
    bool useCtrlCStub;

    // For Debugger testing.
    QObject *testReceiver;
    const char *testCallback;
    int testCase;
};

namespace Internal {

bool fillParameters(DebuggerStartParameters *sp, const ProjectExplorer::Kit *kit, QString *errorMessage = 0);

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::DebuggerStartParameters)

#endif // DEBUGGER_DEBUGGERSTARTPARAMETERS_H
