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

#include "cmakebuildsettingswidget.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildinfo.h"
#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/detailswidget.h>

#include <QFormLayout>

namespace CMakeProjectManager {
namespace Internal {

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    m_pathLineEdit(new QLineEdit),
    m_changeButton(new QPushButton)
{
    auto vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    auto container = new Utils::DetailsWidget;
    container->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(container);

    auto details = new QWidget(container);
    container->setWidget(details);

    auto fl = new QFormLayout(details);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto runCmakeButton = new QPushButton(tr("Run CMake..."));
    connect(runCmakeButton, &QAbstractButton::clicked, this, &CMakeBuildSettingsWidget::runCMake);
    fl->addRow(tr("Reconfigure project:"), runCmakeButton);

    m_pathLineEdit->setReadOnly(true);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(m_pathLineEdit);

    m_changeButton->setText(tr("&Change"));
    connect(m_changeButton, &QAbstractButton::clicked, this,
            &CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog);
    hbox->addWidget(m_changeButton);

    fl->addRow(tr("Build directory:"), hbox);

    m_buildConfiguration = bc;
    m_pathLineEdit->setText(m_buildConfiguration->rawBuildDirectory().toString());
    if (m_buildConfiguration->buildDirectory() == bc->target()->project()->projectDirectory())
        m_changeButton->setEnabled(false);
    else
        m_changeButton->setEnabled(true);

    setDisplayName(tr("CMake"));
}

void CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog()
{
    CMakeBuildInfo info(m_buildConfiguration);
    CMakeOpenProjectWizard copw(Core::ICore::mainWindow(), CMakeOpenProjectWizard::ChangeDirectory,
                                &info);
    if (copw.exec() == QDialog::Accepted) {
        auto project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
        project->changeBuildDirectory(m_buildConfiguration, copw.buildDirectory());
        m_pathLineEdit->setText(m_buildConfiguration->rawBuildDirectory().toString());
    }
}

void CMakeBuildSettingsWidget::runCMake()
{
    if (!ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
        return;
    CMakeBuildInfo info(m_buildConfiguration);
    CMakeOpenProjectWizard copw(Core::ICore::mainWindow(), CMakeOpenProjectWizard::WantToUpdate,
                                &info);
    if (copw.exec() == QDialog::Accepted) {
        auto project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
        project->parseCMakeLists();
    }
}
} // namespace Internal
} // namespace CMakeProjectManager
