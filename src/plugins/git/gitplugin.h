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

#ifndef GITPLUGIN_H
#define GITPLUGIN_H

#include "gitsettings.h"

#include <coreplugin/id.h>

#include <vcsbase/vcsbaseplugin.h>

#include <QStringList>
#include <QPointer>
#include <QKeySequence>
#include <QVector>

#include <functional>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class Command;
class CommandLocator;
class Context;
class ActionContainer;
}
namespace Utils { class ParameterAction; }
namespace Gerrit {
namespace Internal { class GerritPlugin; }
}
namespace Git {
namespace Internal {

class GitVersionControl;
class GitClient;
class GitSubmitEditor;
class CommitData;
class StashDialog;
class BranchDialog;
class RemoteDialog;

typedef void (GitClient::*GitClientMemberFunc)(const QString &);

class GitPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Git.json")

public:
    GitPlugin();
    ~GitPlugin();

    static GitPlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage) override;

    GitVersionControl *gitVersionControl() const;

    GitClient *client() const;
    Gerrit::Internal::GerritPlugin *gerritPlugin() const;
    bool isCommitEditorOpen() const;

public slots:
    void startCommit();
    void updateBranches(const QString &repository);

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState) override;
    bool submitEditorAboutToClose() override;

#ifdef WITH_TESTS
private slots:
    void testStatusParsing_data();
    void testStatusParsing();
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

private:
    void diffCurrentFile();
    void diffCurrentProject();
    void submitCurrentLog();
    void logFile();
    void blameFile();
    void logProject();
    void logRepository();
    void undoFileChanges(bool revertStaging);
    void resetRepository();
    void startRebase();
    void startChangeRelatedAction(const Core::Id &id);
    void stageFile();
    void unstageFile();
    void gitkForCurrentFile();
    void gitkForCurrentFolder();
    void gitGui();
    void cleanProject();
    void cleanRepository();
    void updateSubmodules();
    void applyCurrentFilePatch();
    void promptApplyPatch();

    void startAmendCommit();
    void startFixupCommit();
    void stash(bool unstagedOnly = false);
    void stashUnstaged();
    void stashSnapshot();
    void stashPop();
    void branchList();
    void remoteList();
    void stashList();
    void fetch();
    void pull();
    void push();
    void startMergeTool();
    void continueOrAbortCommand();
    void updateContinueAndAbortCommands();
    void delayedPushToGerrit();

    Core::Command *createCommand(QAction *action, Core::ActionContainer *ac, Core::Id id,
                                 const Core::Context &context, bool addToLocator,
                                 const std::function<void()> &callback, const QKeySequence &keys);

    Utils::ParameterAction *createParameterAction(Core::ActionContainer *ac,
                                                  const QString &defaultText, const QString &parameterText,
                                                  Core::Id id, const Core::Context &context, bool addToLocator,
                                                  const std::function<void()> &callback,
                                                  const QKeySequence &keys = QKeySequence());

    QAction *createFileAction(Core::ActionContainer *ac,
                              const QString &defaultText, const QString &parameterText,
                              Core::Id id, const Core::Context &context, bool addToLocator,
                              const std::function<void()> &callback,
                              const QKeySequence &keys = QKeySequence());
    QAction *createFileAction(Core::ActionContainer *ac,
                              const QString &defaultText, const QString &parameterText,
                              Core::Id id, const Core::Context &context, bool addToLocator,
                              void (GitPlugin::*func)(),
                              const QKeySequence &keys = QKeySequence());

    QAction *createProjectAction(Core::ActionContainer *ac,
                                 const QString &defaultText, const QString &parameterText,
                                 Core::Id id, const Core::Context &context, bool addToLocator,
                                 void (GitPlugin::*func)(),
                                 const QKeySequence &keys = QKeySequence());

    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Core::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    const std::function<void()> &callback,
                                    const QKeySequence &keys = QKeySequence());
    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Core::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    GitClientMemberFunc, const QKeySequence &keys = QKeySequence());

    QAction *createChangeRelatedRepositoryAction(const QString &text, Core::Id id,
                                                 const Core::Context &context);

    void updateRepositoryBrowserAction();
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();
    void cleanRepository(const QString &directory);
    void applyPatch(const QString &workingDirectory, QString file = QString());
    void startCommit(CommitType commitType);
    void updateVersionWarning();

    Core::CommandLocator *m_commandLocator;

    QAction *m_submitCurrentAction;
    QAction *m_diffSelectedFilesAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_menuAction;
    QAction *m_repositoryBrowserAction;
    QAction *m_mergeToolAction;
    QAction *m_submoduleUpdateAction;
    QAction *m_abortMergeAction;
    QAction *m_abortRebaseAction;
    QAction *m_abortCherryPickAction;
    QAction *m_abortRevertAction;
    QAction *m_continueRebaseAction;
    QAction *m_continueCherryPickAction;
    QAction *m_continueRevertAction;
    QAction *m_fixupCommitAction;
    QAction *m_interactiveRebaseAction;

    QVector<Utils::ParameterAction *> m_fileActions;
    QVector<Utils::ParameterAction *> m_projectActions;
    QVector<QAction *> m_repositoryActions;
    Utils::ParameterAction *m_applyCurrentFilePatchAction;
    Gerrit::Internal::GerritPlugin *m_gerritPlugin;

    GitClient *m_gitClient;
    QPointer<StashDialog> m_stashDialog;
    QPointer<BranchDialog> m_branchDialog;
    QPointer<RemoteDialog> m_remoteDialog;
    QString m_submitRepository;
    QString m_commitMessageFileName;
    bool m_submitActionTriggered;
};

} // namespace Internal
} // namespace Git

#endif // GITPLUGIN_H
