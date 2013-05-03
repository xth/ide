/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "lldbengine.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerplugin.h"
#include "debuggerprotocol.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "disassemblerlines.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "sourceutils.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVariant>

#include <QApplication>
#include <QMessageBox>
#include <QToolTip>

#include <stdio.h>


namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// LldbEngine
//
///////////////////////////////////////////////////////////////////////

LldbEngine::LldbEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("LldbEngine"));
    m_disassemblerAgent = 0;
}

LldbEngine::~LldbEngine()
{}

void LldbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (!(languages & CppLanguage))
        return;
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    if (state() == DebuggerNotReady) {
        showMessage(_("LLDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }
    //runCommand(command.toUtf8());
    m_lldbProc.write(command.toUtf8() + '\n');
}

static int token = 1;

void LldbEngine::runCommand(const QByteArray &functionName,
    const QByteArray &extraArgs, const QByteArray &continuation)
{
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    ++token;
    QByteArray cmd = "db ['" + functionName + "',"
            + QByteArray::number(token) + ","
            + "'" + extraArgs + "',"
            + "'" + continuation + "']\n";
    showMessage(QString::number(token) + _(cmd), LogInput);
    m_lldbProc.write(cmd);
}

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    notifyInferiorShutdownOk();
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_lldbProc.kill();
}

void LldbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_lldb = startParameters().debuggerCommand;
    showMessage(_("STARTING LLDB ") + m_lldb);

    connect(&m_lldbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleLldbError(QProcess::ProcessError)));
    connect(&m_lldbProc, SIGNAL(finished(int,QProcess::ExitStatus)),
        SLOT(handleLldbFinished(int,QProcess::ExitStatus)));
    connect(&m_lldbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readLldbStandardOutput()));
    connect(&m_lldbProc, SIGNAL(readyReadStandardError()),
        SLOT(readLldbStandardError()));

    connect(this, SIGNAL(outputReady(QByteArray)),
        SLOT(handleResponse(QByteArray)), Qt::QueuedConnection);

    //m_lldbProc.start(m_lldb);
    m_lldbProc.start(_("/usr/bin/python"), QStringList() << _("-i") << m_lldb);

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start lldb '%1': %2")
            .arg(m_lldb, m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty())
            Core::ICore::showWarningWithOptions(tr("Adapter start failed"), msg);
    }
}

void LldbEngine::setupInferior()
{
    QString fileName = QFileInfo(startParameters().executable).absoluteFilePath();
    runCommand("setupInferior", '"' + fileName.toUtf8() + '"');
}

void LldbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    QByteArray command;
    bool done = attemptBreakpointSynchronizationHelper(&command);
    if (done)
        runEngine2();
    else
        runCommand("handleBreakpoints", command, "runEngine2");
}

void LldbEngine::runEngine2()
{
    showStatusMessage(tr("Running requested..."), 5000);
    runCommand("runEngine");
}

void LldbEngine::interruptInferior()
{
    showStatusMessage(tr("Interrupt requested..."), 5000);
    runCommand("interrupt");
}

void LldbEngine::executeStep()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("step");
}

void LldbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("stepi");
}

void LldbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("finish");
}

void LldbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("next");
}

void LldbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("nexti");
}

void LldbEngine::continueInferior()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("continue");
}

void LldbEngine::handleResponse(const QByteArray &response)
{
    GdbMi all;
    all.fromStringMultiple(response);

    foreach (const GdbMi &item, all.children()) {
        const QByteArray name = item.name();
        if (name == "data")
            refreshLocals(item);
        else if (name == "stack")
            refreshStack(item);
        else if (name == "registers")
            refreshRegisters(item);
        else if (name == "threads")
            refreshThreads(item);
        else if (name == "typeinfo")
            refreshTypeInfo(item);
        else if (name == "state")
            refreshState(item);
        else if (name == "location")
            refreshLocation(item);
        else if (name == "modules")
            refreshModules(item);
        else if (name == "bkpts")
            refreshBreakpoints(item);
        else if (name == "disassembly")
            refreshDisassembly(item);
        else if (name == "continuation")
            runContinuation(item);
    }
}

void LldbEngine::runContinuation(const GdbMi &data)
{
    const QByteArray target = data.data();
    QMetaObject::invokeMethod(this, target, Qt::QueuedConnection);
}

void LldbEngine::executeRunToLine(const ContextData &data)
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeRunToLine", QByteArray::number(data.address)
               + ",'" + data.fileName.toUtf8() + "'");
}

void LldbEngine::executeRunToFunction(const QString &functionName)
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeRunToFunction", '"' + functionName.toUtf8() + '"');
}

void LldbEngine::executeJumpToLine(const ContextData &data)
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeJumpToLine", QByteArray::number(data.address)
        + ',' + data.fileName.toUtf8());
}

void LldbEngine::activateFrame(int frameIndex)
{
    resetLocation();
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    runCommand("activateFrame", QByteArray::number(frameIndex));
}

void LldbEngine::selectThread(ThreadId threadId)
{
    runCommand("selectThread", QByteArray::number(threadId.raw()));
}

bool LldbEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    return breakHandler()->breakpointData(id).isCppBreakpoint()
        && startParameters().startMode != AttachCore;
}

bool LldbEngine::attemptBreakpointSynchronizationHelper(QByteArray *command)
{
    BreakHandler *handler = breakHandler();

    foreach (BreakpointModelId id, handler->unclaimedBreakpointIds()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(id)) {
            showMessage(_("TAKING OWNERSHIP OF BREAKPOINT %1 IN STATE %2")
                .arg(id.toString()).arg(handler->state(id)));
            handler->setEngine(id, this);
        } else {
            showMessage(_("BREAKPOINT %1 IN STATE %2 IS NOT ACCEPTABLE")
                .arg(id.toString()).arg(handler->state(id)));
        }
    }

    QByteArray toAdd;
    QByteArray toChange;
    QByteArray toRemove;

    bool done = true;
    foreach (BreakpointModelId id, handler->engineBreakpointIds(this)) {
        const BreakpointResponse &response = handler->response(id);
        switch (handler->state(id)) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_CHECK(false);
            break;
        case BreakpointInsertRequested:
            done = false;
            toAdd += "{\"modelid\":" + id.toByteArray();
            toAdd += ",\"type\":" + QByteArray::number(handler->type(id));
            toAdd += ",\"ignorecount\":" + QByteArray::number(handler->ignoreCount(id));
            toAdd += ",\"condition\":\"" + handler->condition(id) + '"';
            toAdd += ",\"function\":\"" + handler->functionName(id).toUtf8() + '"';
            toAdd += ",\"oneshot\":" + QByteArray::number(int(handler->isOneShot(id)));
            toAdd += ",\"enabled\":" + QByteArray::number(int(handler->isEnabled(id)));
            toAdd += ",\"file\":\"" + handler->fileName(id).toUtf8() + '"';
            toAdd += ",\"line\":" + QByteArray::number(handler->lineNumber(id)) + "},";
            handler->notifyBreakpointInsertProceeding(id);
            break;
        case BreakpointChangeRequested:
            done = false;
            toChange += "{\"modelid\":" + id.toByteArray();
            toChange += ",\"lldbid\":" + response.id.toByteArray();
            toChange += ",\"type\":" + QByteArray::number(handler->type(id));
            toChange += ",\"ignorecount\":" + QByteArray::number(handler->ignoreCount(id));
            toChange += ",\"condition\":\"" + handler->condition(id) + '"';
            toChange += ",\"function\":\"" + handler->functionName(id).toUtf8() + '"';
            toChange += ",\"oneshot\":" + QByteArray::number(int(handler->isOneShot(id)));
            toChange += ",\"enabled\":" + QByteArray::number(int(handler->isEnabled(id)));
            toChange += ",\"file\":\"" + handler->fileName(id).toUtf8() + '"';
            toChange += ",\"line\":" + QByteArray::number(handler->lineNumber(id)) + "},";
            handler->notifyBreakpointChangeProceeding(id);
            break;
        case BreakpointRemoveRequested:
            done = false;
            toRemove += "{\"modelid\":" + id.toByteArray();
            toRemove += ",\"lldbid\":" + response.id.toByteArray() + "},";
            handler->notifyBreakpointRemoveProceeding(id);
            break;
        case BreakpointChangeProceeding:
        case BreakpointInsertProceeding:
        case BreakpointRemoveProceeding:
        case BreakpointInserted:
        case BreakpointDead:
            QTC_CHECK(false);
            break;
        default:
            QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << id << state());
        }
    }
    if (!done)
        *command = '[' + toAdd + "],[" + toChange + "],[" + toRemove + ']';
    return done;
}

void LldbEngine::attemptBreakpointSynchronization()
{
    showMessage(_("ATTEMPT BREAKPOINT SYNCHRONIZATION"));
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    QByteArray command;
    bool done = attemptBreakpointSynchronizationHelper(&command);
    if (!done) {
        showMessage(_("BREAKPOINTS ARE NOT FULLY SYNCHRONIZED"));
        runCommand("handleBreakpoints", command);
    } else {
        showMessage(_("BREAKPOINTS ARE SYNCHRONIZED"));
    }
}

void LldbEngine::updateBreakpointData(const GdbMi &bkpt, bool added)
{
    BreakHandler *handler = breakHandler();
    BreakpointModelId id = BreakpointModelId(bkpt.findChild("modelid").data());
    BreakpointResponse response = handler->response(id);
    BreakpointResponseId rid = BreakpointResponseId(bkpt.findChild("lldbid").data());
    if (added)
        response.id = rid;
    QTC_CHECK(response.id == rid);
    response.address = 0;
    response.enabled = bkpt.findChild("enabled").data().toInt();
    response.ignoreCount = bkpt.findChild("ignorecount").data().toInt();
    response.condition = bkpt.findChild("condition").data();
    response.hitCount = bkpt.findChild("hitcount").data().toInt();

    if (added) {
        // Added.
        GdbMi locations = bkpt.findChild("locations");
        const int numChild = locations.children().size();
        if (numChild > 1) {
            foreach (const GdbMi &location, locations.children()) {
                BreakpointResponse sub;
                sub.id = BreakpointResponseId(rid.majorPart(),
                    location.findChild("subid").data().toUShort());
                sub.type = response.type;
                sub.address = location.findChild("addr").toAddress();
                sub.functionName = QString::fromUtf8(location.findChild("func").data());
                handler->insertSubBreakpoint(id, sub);
            }
        } else if (numChild == 1) {
            const GdbMi location = locations.childAt(0);
            response.address = location.findChild("addr").toAddress();
            response.functionName = QString::fromUtf8(location.findChild("func").data());
        } else {
            QTC_CHECK(false);
        }
        handler->setResponse(id, response);
        handler->notifyBreakpointInsertOk(id);
    } else {
        // Changed.
        handler->setResponse(id, response);
        handler->notifyBreakpointChangeOk(id);
    }
}

void LldbEngine::refreshDisassembly(const GdbMi &lines)
{
    DisassemblerLines result;

    foreach (const GdbMi &line, lines.children()) {
        DisassemblerLine dl;
        QByteArray address = line.findChild("address").data();
        dl.address = address.toULongLong(0, 0);
        dl.data = _(line.findChild("inst").data());
        dl.function = _(line.findChild("func-name").data());
        dl.offset = line.findChild("offset").data().toUInt();
        result.appendLine(dl);
    }

    QTC_ASSERT(m_disassemblerAgent, return);
    m_disassemblerAgent->setContents(result);
    m_disassemblerAgent = 0;
}

void LldbEngine::refreshBreakpoints(const GdbMi &bkpts)
{
    BreakHandler *handler = breakHandler();
    GdbMi added = bkpts.findChild("added");
    GdbMi changed = bkpts.findChild("changed");
    GdbMi removed = bkpts.findChild("removed");
    foreach (const GdbMi &bkpt, added.children()) {
        BreakpointModelId id = BreakpointModelId(bkpt.findChild("modelid").data());
        QTC_CHECK(handler->state(id) == BreakpointInsertProceeding);
        updateBreakpointData(bkpt, true);
    }
    foreach (const GdbMi &bkpt, changed.children()) {
        BreakpointModelId id = BreakpointModelId(bkpt.findChild("modelid").data());
        QTC_CHECK(handler->state(id) == BreakpointChangeProceeding);
        updateBreakpointData(bkpt, false);
    }
    foreach (const GdbMi &bkpt, removed.children()) {
        BreakpointModelId id = BreakpointModelId(bkpt.findChild("modelid").data());
        QTC_CHECK(handler->state(id) == BreakpointRemoveProceeding);
        handler->notifyBreakpointRemoveOk(id);
    }
}

void LldbEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void LldbEngine::loadAllSymbols()
{
}

void LldbEngine::reloadModules()
{
    runCommand("listModules");
}

void LldbEngine::refreshModules(const GdbMi &modules)
{
    Modules mods;
    foreach (const GdbMi &item, modules.children()) {
        Module module;
        module.modulePath = QString::fromUtf8(item.findChild("file").data());
        module.moduleName = QString::fromUtf8(item.findChild("name").data());
        module.symbolsRead = Module::UnknownReadState;
        module.startAddress = 0;
        //    item.findChild("loaded_addr").data().toULongLong(0, 0);
        module.endAddress = 0; // FIXME: End address not easily available.
        //modulesHandler()->updateModule(module);
        mods.append(module);
    }
    modulesHandler()->setModules(mods);
}

void LldbEngine::requestModuleSymbols(const QString &moduleName)
{
    runCommand("requestModuleSymbols", '"' + moduleName.toUtf8() + '"');
}

void LldbEngine::handleListSymbols(const QByteArray &response)
{
    Q_UNUSED(response);
//    GdbMi out;
//    out.fromString(response.trimmed());
//    Symbols symbols;
//    QString moduleName = response.cookie.toString();
//    foreach (const GdbMi &item, out.children()) {
//        Symbol symbol;
//        symbol.name = _(item.findChild("name").data());
//        symbols.append(symbol);
//    }
//   debuggerCore()->showModuleSymbols(moduleName, symbols);
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

bool LldbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    Q_UNUSED(mousePos);
    Q_UNUSED(editor);

    if (state() != InferiorStopOk) {
        //SDEBUG("SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED");
        return false;
    }
    // Check mime type and get expression (borrowing some C++ - functions)
    const QString javaPythonMimeType =
        QLatin1String("application/javascript");
    if (!editor->document() || editor->document()->mimeType() != javaPythonMimeType)
        return false;

    int line;
    int column;
    QString exp = cppExpressionAt(editor, ctx.position, &line, &column);

/*
    if (m_toolTipCache.contains(exp)) {
        const WatchData & data = m_toolTipCache[exp];
        q->watchHandler()->removeChildren(data.iname);
        insertData(data);
        return;
    }
*/

    QToolTip::hideText();
    if (exp.isEmpty() || exp.startsWith(QLatin1Char('#')))  {
        QToolTip::hideText();
        return false;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos, tr("'%1' contains no identifier").arg(exp));
        return true;
    }

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"'))) {
        QToolTip::showText(m_toolTipPos, tr("String literal %1").arg(exp));
        return true;
    }

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return false;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            tr("Cowardly refusing to evaluate expression '%1' "
               "with potential side effects").arg(exp));
        return true;
    }

#if 0
    //if (status() != InferiorStopOk)
    //    return;

    // FIXME: 'exp' can contain illegal characters
    m_toolTip = WatchData();
    m_toolTip.exp = exp;
    m_toolTip.name = exp;
    m_toolTip.iname = tooltipIName;
    insertData(m_toolTip);
#endif
    return false;
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void LldbEngine::assignValueInDebugger(const Internal::WatchData *, const QString &expression, const QVariant &value)
{
    Q_UNUSED(expression);
    Q_UNUSED(value);
    //SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value.toString()));
#if 0
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value.toString());
    updateLocals();
#endif
}


void LldbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(data);
    Q_UNUSED(flags);
    WatchHandler *handler = watchHandler();
    runCommand("bb", "{'expanded':'" + handler->expansionRequests() + "'}");
}

void LldbEngine::handleLldbError(QProcess::ProcessError error)
{
    qDebug() << "HANDLE LLDB ERROR";
    showMessage(_("HANDLE LLDB ERROR"));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        m_lldbProc.kill();
        showMessageBox(QMessageBox::Critical, tr("Lldb I/O Error"),
                       errorMessage(error));
        break;
    }
}

QString LldbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Lldb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(m_lldb);
        case QProcess::Crashed:
            return tr("The Lldb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Lldb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Lldb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Lldb process occurred. ");
    }
}

void LldbEngine::handleLldbFinished(int code, QProcess::ExitStatus type)
{
    qDebug() << "LLDB FINISHED";
    showMessage(_("LLDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    notifyEngineSpontaneousShutdown();
}

void LldbEngine::readLldbStandardError()
{
    QByteArray err = m_lldbProc.readAllStandardError();
    qDebug() << "\nLLDB STDERR" << err;
    //qWarning() << "Unexpected lldb stderr:" << err;
    showMessage(_("Lldb stderr: " + err));
    //handleOutput(err);
}

void LldbEngine::readLldbStandardOutput()
{
    QByteArray out = m_lldbProc.readAllStandardOutput();
    showMessage(_("Lldb stdout: " + out));
    //qDebug("\nLLDB RAW STDOUT: '%s'", quoteUnprintable(out).constData());
    //qDebug("\nBUFFER FROM: '%s'", quoteUnprintable(m_inbuffer).constData());
    m_inbuffer.append(out);
    while (true) {
        int pos = m_inbuffer.indexOf("@\n");
        if (pos == -1)
            break;
        QByteArray response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 2);
        //qDebug("\nBUFFER RECOGNIZED: '%s'", quoteUnprintable(response).constData());
        //showMessage(_(response));
        emit outputReady(response);
    }
    //qDebug("\nBUFFER LEFT: '%s'", quoteUnprintable(m_inbuffer).constData());
}

QByteArray LldbEngine::currentOptions() const
{
    QByteArray localsOptions;
    QByteArray stackOptions;
    QByteArray threadsOptions;

    {
        QByteArray watchers;
        //if (!m_toolTipExpression.isEmpty())
        //    watchers += m_toolTipExpression.toLatin1()
        //        + '#' + tooltipINameForExpression(m_toolTipExpression.toLatin1());

        WatchHandler *handler = watchHandler();
        QHash<QByteArray, int> watcherNames = handler->watcherNames();
        QHashIterator<QByteArray, int> it(watcherNames);
        while (it.hasNext()) {
            it.next();
            if (!watchers.isEmpty())
                watchers += "##";
            watchers += it.key() + "#watch." + QByteArray::number(it.value());
        }

        QByteArray options;
        if (debuggerCore()->boolSetting(UseDebuggingHelpers))
            options += "fancy,";
        if (debuggerCore()->boolSetting(AutoDerefPointers))
            options += "autoderef,";
        if (options.isEmpty())
            options += "defaults,";
        options.chop(1);

        localsOptions = "options:" + options + " "
            + "vars: "
            + "expanded:" + handler->expansionRequests() + " "
            + "typeformats:" + handler->typeFormatRequests() + " "
            + "formats:" + handler->individualFormatRequests() + " "
            + "watcher:" + watchers.toHex();
    }

    {
        int maxdepth = debuggerCore()->action(MaximalStackDepth)->value().toInt();
        ThreadId curthread = threadsHandler()->currentThread();
        stackOptions += "maxdepth:" + QByteArray::number(maxdepth);
        stackOptions += ",curthread:" + QByteArray::number(curthread.raw());
    }

    QByteArray result;
    result += "\"locals\":\"" + localsOptions + '"';
    result += ",\"stack\":\"" + stackOptions + '"';
    result += ",\"threads\":\"" + threadsOptions + '"';

    return result;
}

void LldbEngine::updateAll()
{
    runCommand("createReport");
}

void LldbEngine::refreshLocals(const GdbMi &vars)
{
    //const bool partial = response.cookie.toBool();
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();
    QList<WatchData> list;

    //if (!partial) {
        list.append(*handler->findData("local"));
        list.append(*handler->findData("watch"));
        list.append(*handler->findData("return"));
    //}

    foreach (const GdbMi &child, vars.children()) {
        WatchData dummy;
        dummy.iname = child.findChild("iname").data();
        GdbMi wname = child.findChild("wname");
        if (wname.isValid()) {
            // Happens (only) for watched expressions. They are encoded as
            // base64 encoded 8 bit data, without quotes
            dummy.name = decodeData(wname.data(), Base64Encoded8Bit);
            dummy.exp = dummy.name.toUtf8();
        } else {
            dummy.name = _(child.findChild("name").data());
        }
        parseWatchData(handler->expandedINames(), dummy, child, &list);
    }
    handler->insertData(list);
 }

void LldbEngine::refreshStack(const GdbMi &stack)
{
    //if (!partial)
    //    emit stackFrameCompleted();
    StackHandler *handler = stackHandler();
    StackFrames frames;
    foreach (const GdbMi &item, stack.findChild("frames").children()) {
        StackFrame frame;
        frame.level = item.findChild("level").data().toInt();
        frame.file = QString::fromLatin1(item.findChild("file").data());
        frame.function = QString::fromLatin1(item.findChild("func").data());
        frame.from = QString::fromLatin1(item.findChild("func").data());
        frame.line = item.findChild("line").data().toInt();
        frame.address = item.findChild("addr").toAddress();
        frame.usable = QFileInfo(frame.file).isReadable();
        frames.append(frame);
    }
    bool canExpand = stack.findChild("hasmore").data().toInt();
    debuggerCore()->action(ExpandStack)->setEnabled(canExpand);
    handler->setFrames(frames);
}

void LldbEngine::refreshRegisters(const GdbMi &registers)
{
    RegisterHandler *handler = registerHandler();
    Registers regs;
    foreach (const GdbMi &item, registers.children()) {
        Register reg;
        reg.name = item.findChild("name").data();
        reg.value = item.findChild("value").data();
        //reg.type = item.findChild("type").data();
        regs.append(reg);
    }
    //handler->setRegisters(registers);
    handler->setAndMarkRegisters(regs);
}

void LldbEngine::refreshThreads(const GdbMi &threads)
{
    ThreadsHandler *handler = threadsHandler();
    handler->updateThreads(threads);
    if (!handler->currentThread().isValid()) {
        ThreadId other = handler->threadAt(0);
        if (other.isValid())
            selectThread(other);
    }
    updateViews(); // Adjust Threads combobox.
}

void LldbEngine::refreshTypeInfo(const GdbMi &typeInfo)
{
    if (typeInfo.type() == GdbMi::List) {
//        foreach (const GdbMi &s, typeInfo.children()) {
//            const GdbMi name = s.findChild("name");
//            const GdbMi size = s.findChild("size");
//            if (name.isValid() && size.isValid())
//                m_typeInfoCache.insert(QByteArray::fromBase64(name.data()),
//                                       TypeInfo(size.data().toUInt()));
//        }
    }
//    for (int i = 0; i != list.size(); ++i) {
//        const TypeInfo ti = m_typeInfoCache.value(list.at(i).type);
//        if (ti.size)
//            list[i].size = ti.size;
//    }
}

void LldbEngine::refreshState(const GdbMi &reportedState)
{
    QByteArray newState = reportedState.data();
    if (newState == "running")
        notifyInferiorRunOk();
    else if (newState == "inferiorrunfailed")
        notifyInferiorRunFailed();
    else if (newState == "stopped")
        notifyInferiorSpontaneousStop();
    else if (newState == "inferiorstopok")
        notifyInferiorStopOk();
    else if (newState == "inferiorstopfailed")
        notifyInferiorStopFailed();
    else if (newState == "enginesetupok")
        notifyEngineSetupOk();
    else if (newState == "enginesetupfailed")
        notifyEngineSetupFailed();
    else if (newState == "stopped")
        notifyInferiorSpontaneousStop();
    else if (newState == "inferiorsetupok")
        notifyInferiorSetupOk();
    else if (newState == "enginerunok")
        notifyEngineRunAndInferiorRunOk();
}

void LldbEngine::refreshLocation(const GdbMi &reportedLocation)
{
    QByteArray file = reportedLocation.findChild("file").data();
    int line = reportedLocation.findChild("line").data().toInt();
    gotoLocation(Location(QString::fromUtf8(file), line));
}

void LldbEngine::reloadRegisters()
{
    runCommand("reloadRegisters");
}

void LldbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    QTC_CHECK(!m_disassemblerAgent);
    m_disassemblerAgent = agent;
    QByteArray cookie = QByteArray::number(qulonglong(agent));
    runCommand("disassemble", "{'cookie':" + cookie + "}");
}

bool LldbEngine::hasCapability(unsigned cap) const
{
    if (cap & (ReverseSteppingCapability
        | AutoDerefPointersCapability
        | DisassemblerCapability
        | RegisterCapability
        | ShowMemoryCapability
        | JumpToLineCapability
        | ReloadModuleCapability
        | ReloadModuleSymbolsCapability
        | BreakOnThrowAndCatchCapability
        | BreakConditionCapability
        | TracePointCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointByAddressCapability
        | WatchpointByExpressionCapability
        | AddWatcherCapability
        | WatchWidgetsCapability
        | ShowModuleSymbolsCapability
        | ShowModuleSectionsCapability
        | CatchCapability
        | OperateByInstructionCapability
        | RunToLineCapability
        | WatchComplexExpressionsCapability
        | MemoryAddressCapability))
        return true;

    if (startParameters().startMode == AttachCore)
        return false;

    //return cap == SnapshotCapability;
    return false;
}

DebuggerEngine *createLldbEngine(const DebuggerStartParameters &startParameters)
{
    return new LldbEngine(startParameters);
}

} // namespace Internal
} // namespace Debugger
