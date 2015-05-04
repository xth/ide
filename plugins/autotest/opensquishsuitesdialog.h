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

#ifndef OPENSQUISHSUITESDIALOG_H
#define OPENSQUISHSUITESDIALOG_H

#include <QDialog>

class QListWidgetItem;

namespace Ui {
class OpenSquishSuitesDialog;
}

namespace Autotest {
namespace Internal {

class OpenSquishSuitesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenSquishSuitesDialog(QWidget *parent = 0);
    ~OpenSquishSuitesDialog();
    QStringList chosenSuites() const { return m_chosenSuites; }

private:
    void onDirectoryChanged();
    void onListItemChanged(QListWidgetItem *);
    void selectAll();
    void deselectAll();
    void setChosenSuites();

    Ui::OpenSquishSuitesDialog *ui;
    QStringList m_chosenSuites;
};

} // namespace Internal
} // namespace Autotest

#endif // OPENSQUISHSUITESDIALOG_H
