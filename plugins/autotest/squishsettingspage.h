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

#ifndef SQUISHSETTINGSPAGE_H
#define SQUISHSETTINGSPAGE_H

#include "ui_squishsettingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>
#include <QSharedPointer>
#include <QWidget>

namespace Autotest {
namespace Internal {

struct SquishSettings;

class SquishSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SquishSettingsWidget(QWidget *parent = 0);

    void setSettings(const SquishSettings &settings);
    SquishSettings settings() const;

private:
    void onLocalToggled(bool checked);

    Ui::SquishSettingsPage m_ui;

};

class SquishSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit SquishSettingsPage(const QSharedPointer<SquishSettings> &settings);

    QWidget *widget();
    void apply();
    void finish() { }

private:
    QSharedPointer<SquishSettings> m_settings;
    QPointer<SquishSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace Autotest

#endif // SQUISHSETTINGSPAGE_H
