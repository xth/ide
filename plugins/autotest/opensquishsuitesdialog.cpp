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

#include "opensquishsuitesdialog.h"
#include "testsquishutils.h"
#include "ui_opensquishsuitesdialog.h"

#include <QDir>
#include <QPushButton>
#include <QListWidgetItem>

namespace Autotest {
namespace Internal {

static QString previousPath;

OpenSquishSuitesDialog::OpenSquishSuitesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenSquishSuitesDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    connect(ui->directoryLineEdit, &Utils::PathChooser::pathChanged,
            this, &OpenSquishSuitesDialog::onDirectoryChanged);
    connect(ui->selectAllPushButton, &QPushButton::clicked,
            this, &OpenSquishSuitesDialog::selectAll);
    connect(ui->deselectAllPushButton, &QPushButton::clicked,
            this, &OpenSquishSuitesDialog::deselectAll);
    connect(this, &OpenSquishSuitesDialog::accepted,
            this, &OpenSquishSuitesDialog::setChosenSuites);

    ui->directoryLineEdit->setPath(previousPath);
}

OpenSquishSuitesDialog::~OpenSquishSuitesDialog()
{
    delete ui;
}

void OpenSquishSuitesDialog::onDirectoryChanged()
{
    ui->suitesListWidget->clear();
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
    QDir baseDir(ui->directoryLineEdit->path());
    if (!baseDir.exists()) {
        return;
    }

    foreach (const QFileInfo &subDir, baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!subDir.baseName().startsWith(QLatin1String("suite_")))
            continue;

        if (TestSquishUtils::validTestCases(subDir.absoluteFilePath()).size()) {
                QListWidgetItem *item = new QListWidgetItem(subDir.baseName(),
                                                            ui->suitesListWidget);
                item->setCheckState(Qt::Checked);
                connect(ui->suitesListWidget, &QListWidget::itemChanged,
                        this, &OpenSquishSuitesDialog::onListItemChanged);
        }
    }
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(ui->suitesListWidget->count());
}

void OpenSquishSuitesDialog::onListItemChanged(QListWidgetItem *)
{
    const int count = ui->suitesListWidget->count();
    for (int row = 0; row < count; ++row) {
        if (ui->suitesListWidget->item(row)->checkState() == Qt::Checked) {
            ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
            return;
        }
    }
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
}

void OpenSquishSuitesDialog::selectAll()
{
    const int count = ui->suitesListWidget->count();
    for (int row = 0; row < count; ++row)
        ui->suitesListWidget->item(row)->setCheckState(Qt::Checked);
}

void OpenSquishSuitesDialog::deselectAll()
{
    const int count = ui->suitesListWidget->count();
    for (int row = 0; row < count; ++row)
        ui->suitesListWidget->item(row)->setCheckState(Qt::Unchecked);
}

void OpenSquishSuitesDialog::setChosenSuites()
{
    const int count = ui->suitesListWidget->count();
    previousPath = ui->directoryLineEdit->path();
    const QDir baseDir(previousPath);
    for (int row = 0; row < count; ++row) {
        QListWidgetItem *item = ui->suitesListWidget->item(row);
        if (item->checkState() == Qt::Checked)
            m_chosenSuites.append(QFileInfo(baseDir, item->text()).absoluteFilePath());
    }
}

} // namespace Internal
} // namespace Autotest
