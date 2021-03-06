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

#ifndef CALLGRINDTOOL_H
#define CALLGRINDTOOL_H

#include <analyzerbase/ianalyzertool.h>

namespace Valgrind {
namespace Internal {

const char CallgrindToolId[] = "Callgrind";
const char CallgrindLocalActionId[] = "Callgrind.Local";
const char CallgrindRemoteActionId[] = "Callgrind.Remote";

class ValgrindRunControl;
const char CALLGRIND_RUN_MODE[] = "CallgrindTool.CallgrindRunMode";

class CallgrindToolPrivate;

class CallgrindTool : public QObject
{
    Q_OBJECT

public:
    CallgrindTool(QObject *parent);
    ~CallgrindTool();

    ValgrindRunControl *createRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    QWidget *createWidgets();

    void handleShowCostsOfFunction();

private:
    CallgrindToolPrivate *d;
};

} // namespace Internal
} // namespace Valgrind

#endif // CALLGRINDTOOL_H
