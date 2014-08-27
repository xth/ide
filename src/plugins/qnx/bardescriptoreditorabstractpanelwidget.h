/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BARDESCRIPTOREDITORABSTRACTPANELWIDGET_H
#define QNX_INTERNAL_BARDESCRIPTOREDITORABSTRACTPANELWIDGET_H

#include <QWidget>

#include "bardescriptordocument.h"

namespace Utils { class PathChooser; }

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSignalMapper;
class QStringListModel;
class QTextEdit;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BarDescriptorEditorAbstractPanelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BarDescriptorEditorAbstractPanelWidget(QWidget *parent = 0);

public slots:
    void setValue(BarDescriptorDocument::Tag tag, const QVariant &value);

signals:
    void changed(BarDescriptorDocument::Tag tag, const QVariant &value);

protected:
    virtual void updateWidgetValue(BarDescriptorDocument::Tag tag, const QVariant &value);
    virtual void emitChanged(BarDescriptorDocument::Tag tag);

    void addSignalMapping(BarDescriptorDocument::Tag tag, QObject *object, const char *signal);
    void blockSignalMapping(BarDescriptorDocument::Tag tag);
    void unblockSignalMapping(BarDescriptorDocument::Tag tag);

private slots:
    void handleSignalMapped(int id);

private:
    QSignalMapper *m_signalMapper;
    QList<BarDescriptorDocument::Tag> m_blockedSignals;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BARDESCRIPTOREDITORABSTRACTPANELWIDGET_H