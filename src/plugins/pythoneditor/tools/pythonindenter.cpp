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

#include "pythonindenter.h"
#include "lexical/pythonscanner.h"

#include <texteditor/tabsettings.h>

#include <QSet>

namespace PythonEditor {

// Tab size hardcoded as PEP8 style guide requires, but can be moved to settings
static const int TAB_SIZE = 4;

PythonIndenter::PythonIndenter()
{
    m_jumpKeywords << QLatin1String("return")
                   << QLatin1String("yield")
                   << QLatin1String("break")
                   << QLatin1String("continue")
                   << QLatin1String("raise")
                   << QLatin1String("pass");
}

PythonIndenter::~PythonIndenter()
{
}

/**
 * @brief Does given character change indentation level?
 * @param ch Any value
 * @return True if character increases indentation level at the next line
 */
bool PythonIndenter::isElectricCharacter(const QChar &ch) const
{
    return (ch == QLatin1Char(':'));
}

/**
 * @brief Indents one block (i.e. one line) of code
 * @param doc Unused
 * @param block Block that represents line
 * @param typedChar Unused
 * @param tabSettings An IDE tabulation settings
 *
 * Usually this function called once when you begin new line of code by pressing
 * Enter. If Indenter reimplements indent() function, than indentBlock() may be
 * called in other cases.
 */
void PythonIndenter::indentBlock(QTextDocument *document,
                                 const QTextBlock &block,
                                 const QChar &typedChar,
                                 const TextEditor::TabSettings &settings)
{
    Q_UNUSED(document);
    Q_UNUSED(typedChar);
    QTextBlock previousBlock = block.previous();
    if (previousBlock.isValid()) {
        QString previousLine = previousBlock.text();
        int indentation = settings.indentationColumn(previousLine);

        if (isElectricLine(previousLine))
            indentation += TAB_SIZE;
        else
            indentation = qMax<int>(0, indentation + getIndentDiff(previousLine));

        settings.indentLine(block, indentation);
    } else {
        // First line in whole document
        settings.indentLine(block, 0);
    }
}

/// @return True if electric character is last non-space character at given string
bool PythonIndenter::isElectricLine(const QString &line) const
{
    if (line.isEmpty())
        return false;

    // trim spaces in 'if True:  '
    int index = line.length() - 1;
    while ((index > 0) && line[index].isSpace())
        --index;

    return isElectricCharacter(line[index]);
}

/// @return negative indent diff if previous line breaks control flow branch
int PythonIndenter::getIndentDiff(const QString &previousLine) const
{
    Internal::Scanner sc(previousLine.constData(), previousLine.length());
    forever {
        Internal::FormatToken tk = sc.read();
        if ((tk.format() == Internal::Format_Keyword) && m_jumpKeywords.contains(sc.value(tk)))
            return -TAB_SIZE;
        if (tk.format() != Internal::Format_Whitespace)
            break;
    }
    return 0;
}

} // namespace PythonEditor
