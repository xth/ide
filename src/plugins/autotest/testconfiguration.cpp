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

#include "testconfiguration.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace Autotest {
namespace Internal {

TestConfiguration::TestConfiguration(const QString &testClass, const QStringList &testCases,
                                     int testCaseCount, QObject *parent)
    : QObject(parent),
      m_testClass(testClass),
      m_testCases(testCases),
      m_testCaseCount(testCaseCount),
      m_unnamedOnly(false),
      m_project(0),
      m_guessedConfiguration(false),
      m_type(TestTypeQt)
{
    if (testCases.size() != 0)
        m_testCaseCount = testCases.size();
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
}

void basicProjectInformation(Project *project, const QString &mainFilePath, QString *proFile,
                             QString *displayName, Project **targetProject)
{
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectInfo(project).projectParts();

    foreach (const CppTools::ProjectPart::Ptr &part, projParts) {
        foreach (const CppTools::ProjectFile currentFile, part->files) {
            if (currentFile.path == mainFilePath) {
                *proFile = part->projectFile;
                *displayName = part->displayName;
                *targetProject = part->project;
                return;
            }
        }
    }
}

void basicProjectInformation(Project *project, const QString &proFile, QString *displayName,
                             Project **targetProject)
{
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectInfo(project).projectParts();

    foreach (const CppTools::ProjectPart::Ptr &part, projParts) {
        if (part->projectFile == proFile) {
            *displayName = part->displayName;
            *targetProject = part->project;
            return;
        }
    }
}

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : 0;
    Kit *kit = target ? target->kit() : 0;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

void TestConfiguration::completeTestInformation()
{
    QTC_ASSERT(!m_mainFilePath.isEmpty() || !m_proFile.isEmpty(), return);

    Project *project = SessionManager::startupProject();
    if (!project)
        return;

    QString targetFile;
    QString targetName;
    QString workDir;
    QString proFile = m_proFile;
    QString displayName;
    QString buildDir;
    Project *targetProject = 0;
    Utils::Environment env;
    bool hasDesktopTarget = false;
    bool guessedRunConfiguration = false;
    setProject(0);

    if (m_proFile.isEmpty())
        basicProjectInformation(project, m_mainFilePath, &proFile, &displayName, &targetProject);
    else
        basicProjectInformation(project, proFile, &displayName, &targetProject);

    Target *target = project->activeTarget();
    if (!target)
        return;

    BuildTargetInfoList appTargets = target->applicationTargets();
    foreach (const BuildTargetInfo &bti, appTargets.list) {
        // some project manager store line/column information as well inside ProjectPart
        if (bti.isValid() && proFile.startsWith(bti.projectFilePath.toString())) {
            targetFile = Utils::HostOsInfo::withExecutableSuffix(bti.targetFilePath.toString());
            targetName = bti.targetName;
            break;
        }
    }

    if (targetProject) {
        if (auto buildConfig = target->activeBuildConfiguration()) {
            const QString buildBase = buildConfig->buildDirectory().toString();
            const QString projBase = targetProject->projectDirectory().toString();
            if (proFile.startsWith(projBase))
                buildDir = QFileInfo(buildBase + proFile.mid(projBase.length())).absolutePath();
        }
    }

    QList<RunConfiguration *> rcs = target->runConfigurations();
    foreach (RunConfiguration *rc, rcs) {
        Runnable runnable = rc->runnable();
        if (isLocal(rc) && runnable.is<StandardRunnable>()) {
            StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
            if (stdRunnable.executable == targetFile) {
                workDir = Utils::FileUtils::normalizePathName(stdRunnable.workingDirectory);
                env = stdRunnable.environment;
                hasDesktopTarget = true;
                break;
            }
        }
    }

    // if we could not figure out the run configuration
    // try to use the run configuration of the parent project
    if (!hasDesktopTarget && targetProject && !targetFile.isEmpty()) {
        if (auto rc = target->activeRunConfiguration()) {
            Runnable runnable = rc->runnable();
            if (isLocal(rc) && runnable.is<StandardRunnable>()) {
                StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
                workDir = Utils::FileUtils::normalizePathName(stdRunnable.workingDirectory);
                env = stdRunnable.environment;
                hasDesktopTarget = true;
                guessedRunConfiguration = true;
            }
        }
    }

    setProFile(proFile);
    setDisplayName(displayName);

    if (hasDesktopTarget) {
        setTargetFile(targetFile);
        setTargetName(targetName);
        setWorkingDirectory(workDir);
        setBuildDirectory(buildDir);
        setEnvironment(env);
        setProject(project);
        setGuessedConfiguration(guessedRunConfiguration);
    }
}


/**
 * @brief sets the test cases for this test configuration.
 *
 * Watch out for special handling of test configurations, because this method also
 * updates the test case count to the current size of \a testCases.
 *
 * @param testCases list of names of the test functions / test cases
 */
void TestConfiguration::setTestCases(const QStringList &testCases)
{
    m_testCases.clear();
    m_testCases << testCases;
    m_testCaseCount = m_testCases.size();
}

void TestConfiguration::setTestCaseCount(int count)
{
    m_testCaseCount = count;
}

void TestConfiguration::setMainFilePath(const QString &mainFile)
{
    m_mainFilePath = mainFile;
}

void TestConfiguration::setTargetFile(const QString &targetFile)
{
    m_targetFile = targetFile;
}

void TestConfiguration::setTargetName(const QString &targetName)
{
    m_targetName = targetName;
}

void TestConfiguration::setProFile(const QString &proFile)
{
    m_proFile = proFile;
}

void TestConfiguration::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDir = workingDirectory;
}

void TestConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    m_buildDir = buildDirectory;
}

void TestConfiguration::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void TestConfiguration::setEnvironment(const Utils::Environment &env)
{
    m_environment = env;
}

void TestConfiguration::setProject(Project *project)
{
    m_project = project;
}

void TestConfiguration::setUnnamedOnly(bool unnamedOnly)
{
    m_unnamedOnly = unnamedOnly;
}

void TestConfiguration::setGuessedConfiguration(bool guessed)
{
    m_guessedConfiguration = guessed;
}

void TestConfiguration::setTestType(TestType type)
{
    m_type = type;
}

} // namespace Internal
} // namespace Autotest
