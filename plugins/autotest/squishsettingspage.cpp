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

#include "autotestconstants.h"
#include "squishsettingspage.h"
#include "squishsettings.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>

namespace Autotest {
namespace Internal {

SquishSettingsWidget::SquishSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
}

void SquishSettingsWidget::setSettings(const SquishSettings &settings)
{
    m_ui.squishPathChooser->setFileName(settings.squishPath);
}

SquishSettings SquishSettingsWidget::settings() const
{
    SquishSettings result;
    result.squishPath = m_ui.squishPathChooser->fileName();
    return result;
}

SquishSettingsPage::SquishSettingsPage(const QSharedPointer<SquishSettings> &settings)
    : m_settings(settings), m_widget(0)
{
    setId("B.Squish");
    setDisplayName(tr("Squish"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
}

QWidget *SquishSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new SquishSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void SquishSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;
    const SquishSettings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::settings());
    }
}

} // namespace Internal
} // namespace Autotest
