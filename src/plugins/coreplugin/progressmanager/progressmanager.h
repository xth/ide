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

#ifndef PROGRESSMANAGER_H
#define PROGRESSMANAGER_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>

#include <QObject>
#include <QFuture>

namespace Core {
class FutureProgress;

namespace Internal { class ProgressManagerPrivate; }

class CORE_EXPORT ProgressManager : public QObject
{
    Q_OBJECT
public:
    enum ProgressFlag {
        KeepOnFinish = 0x01,
        ShowInApplicationIcon = 0x02
    };
    Q_DECLARE_FLAGS(ProgressFlags, ProgressFlag)

    static ProgressManager *instance();

    static FutureProgress *addTask(const QFuture<void> &future, const QString &title,
                                   Id type, ProgressFlags flags = 0);
    static FutureProgress *addTimedTask(const QFutureInterface<void> &fi, const QString &title,
                                   Id type, int expectedSeconds, ProgressFlags flags = 0);
    static void setApplicationLabel(const QString &text);

public slots:
    static void cancelTasks(Id type);

signals:
    void taskStarted(Core::Id type);
    void allTasksFinished(Core::Id type);

private:
    ProgressManager();
    ~ProgressManager();

    friend class Core::Internal::ProgressManagerPrivate;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::ProgressManager::ProgressFlags)

#endif //PROGRESSMANAGER_H
