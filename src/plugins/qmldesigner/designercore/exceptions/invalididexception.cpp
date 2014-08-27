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

#include "invalididexception.h"

#include <QCoreApplication>

namespace QmlDesigner {

InvalidIdException::InvalidIdException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &id,
                                       Reason reason) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(id)
{
    if (reason == InvalidCharacters)
        m_description = QCoreApplication::translate("InvalidIdException", "Only alphanumeric characters and underscore allowed.\nIds must begin with a lowercase letter.");
    else
        m_description = QCoreApplication::translate("InvalidIdException", "Ids have to be unique.");
}

InvalidIdException::InvalidIdException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &id,
                                       const QString &description) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(id),
    m_description(description)
{
    createWarning();
}

QString InvalidIdException::type() const
{
    return "InvalidIdException";
}

QString InvalidIdException::description() const
{
    return QCoreApplication::translate("InvalidIdException", "Invalid Id: %1\n%2").arg(m_id, m_description);
}

}