/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickabstractmessagedialog_p.h"
#include <QtGui/qpa/qplatformdialoghelper.h>

QT_BEGIN_NAMESPACE

QQuickAbstractMessageDialog::QQuickAbstractMessageDialog(QObject *parent)
    : QQuickAbstractDialog(parent)
    , m_dlgHelper(0)
    , m_options(QSharedPointer<QMessageDialogOptions>(new QMessageDialogOptions()))
    , m_clickedButton(NoButton)
{
}

QQuickAbstractMessageDialog::~QQuickAbstractMessageDialog()
{
}

void QQuickAbstractMessageDialog::setVisible(bool v)
{
    if (helper() && v)
        m_dlgHelper->setOptions(m_options);
    if (v)
        m_clickedButton = NoButton;
    QQuickAbstractDialog::setVisible(v);
}

void QQuickAbstractMessageDialog::setTitle(const QString &arg)
{
    if (arg != m_options->windowTitle()) {
        m_options->setWindowTitle(arg);
        emit titleChanged();
    }
}

void QQuickAbstractMessageDialog::setText(const QString &arg)
{
    if (arg != m_options->text()) {
        m_options->setText(arg);
        emit textChanged();
    }
}

void QQuickAbstractMessageDialog::setInformativeText(const QString &arg)
{
    if (arg != m_options->informativeText()) {
        m_options->setInformativeText(arg);
        emit informativeTextChanged();
    }
}

void QQuickAbstractMessageDialog::setDetailedText(const QString &arg)
{
    if (arg != m_options->detailedText()) {
        m_options->setDetailedText(arg);
        emit detailedTextChanged();
    }
}

void QQuickAbstractMessageDialog::setIcon(QQuickAbstractMessageDialog::Icon icon)
{
    if (static_cast<int>(icon) != static_cast<int>(m_options->icon())) {
        m_options->setIcon(static_cast<QMessageDialogOptions::Icon>(icon));
        emit iconChanged();
    }
}

QUrl QQuickAbstractMessageDialog::standardIconSource()
{
    switch (m_options->icon()) {
    case QMessageDialogOptions::Information:
        return QUrl("images/information.png");
        break;
    case QMessageDialogOptions::Warning:
        return QUrl("images/warning.png");
        break;
    case QMessageDialogOptions::Critical:
        return QUrl("images/critical.png");
        break;
    case QMessageDialogOptions::Question:
        return QUrl("images/question.png");
        break;
    default:
        return QUrl();
        break;
    }
}

void QQuickAbstractMessageDialog::setStandardButtons(StandardButtons buttons)
{
    if (buttons != m_options->standardButtons()) {
        m_options->setStandardButtons(static_cast<QMessageDialogOptions::StandardButtons>(static_cast<int>(buttons)));
        emit standardButtonsChanged();
    }
}

void QQuickAbstractMessageDialog::click(QMessageDialogOptions::StandardButton button, QMessageDialogOptions::ButtonRole role)
{
    setVisible(false);
    m_clickedButton = static_cast<StandardButton>(button);
    emit buttonClicked();
    switch (role) {
    case QMessageDialogOptions::AcceptRole:
        emit accept();
        break;
    case QMessageDialogOptions::RejectRole:
        emit reject();
        break;
    case QMessageDialogOptions::DestructiveRole:
        emit discard();
        break;
    case QMessageDialogOptions::HelpRole:
        emit help();
        break;
    case QMessageDialogOptions::YesRole:
        emit yes();
        break;
    case QMessageDialogOptions::NoRole:
        emit no();
        break;
    case QMessageDialogOptions::ApplyRole:
        emit apply();
        break;
    case QMessageDialogOptions::ResetRole:
        emit reset();
        break;
    default:
        qWarning("unhandled MessageDialog button %d with role %d", button, role);
    }
}

void QQuickAbstractMessageDialog::click(QQuickAbstractMessageDialog::StandardButton button)
{
    click(static_cast<QMessageDialogOptions::StandardButton>(button),
        static_cast<QMessageDialogOptions::ButtonRole>(
            QMessageDialogOptions::buttonRole(static_cast<QMessageDialogOptions::StandardButton>(button))));
}

QT_END_NAMESPACE
