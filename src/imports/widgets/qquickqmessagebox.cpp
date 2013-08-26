/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickqmessagebox_p.h"
#include "qquickitem.h"

#include <private/qguiapplication_p.h>
#include <private/qqmlcontext_p.h>
#include <QWindow>
#include <QQuickWindow>
#include <QMessageBox>
#include <QAbstractButton>

QT_BEGIN_NAMESPACE

class QMessageBoxHelper : public QPlatformMessageDialogHelper
{
public:
    QMessageBoxHelper() {
        connect(&m_dialog, SIGNAL(accepted()), this, SIGNAL(accept()));
        connect(&m_dialog, SIGNAL(rejected()), this, SIGNAL(reject()));
    }

    virtual void exec() { m_dialog.exec(); }

    virtual bool show(Qt::WindowFlags f, Qt::WindowModality m, QWindow *parent) {
        m_dialog.winId();
        QWindow *window = m_dialog.windowHandle();
        Q_ASSERT(window);
        window->setTransientParent(parent);
        window->setFlags(f);
        m_dialog.setWindowModality(m);
        m_dialog.setWindowTitle(QPlatformMessageDialogHelper::options()->windowTitle());
        m_dialog.setIcon(static_cast<QMessageBox::Icon>(QPlatformMessageDialogHelper::options()->icon()));
        if (!QPlatformMessageDialogHelper::options()->text().isNull())
            m_dialog.setText(QPlatformMessageDialogHelper::options()->text());
        if (!QPlatformMessageDialogHelper::options()->informativeText().isNull())
            m_dialog.setInformativeText(QPlatformMessageDialogHelper::options()->informativeText());
        if (!QPlatformMessageDialogHelper::options()->detailedText().isNull())
            m_dialog.setDetailedText(QPlatformMessageDialogHelper::options()->detailedText());
        m_dialog.setStandardButtons(static_cast<QMessageBox::StandardButtons>(static_cast<int>(
            QPlatformMessageDialogHelper::options()->standardButtons())));
        m_dialog.show();
        return m_dialog.isVisible();
    }

    virtual void hide() { m_dialog.hide(); }

    QMessageBox m_dialog;
};

/*!
    \qmltype QtMessageDialog
    \instantiates QQuickQMessageBox
    \inqmlmodule QtQuick.PrivateWidgets 1
    \ingroup qtquick-visual
    \brief Dialog component for choosing a color.
    \since 5.2
    \internal

    QtMessageDialog provides a means to instantiate and manage a QMessageBox.
    It is not recommended to be used directly; it is an implementation
    detail of \l MessageDialog in the \l QtQuick.Dialogs module.

    To use this type, you will need to import the module with the following line:
    \code
    import QtQuick.PrivateWidgets 1.1
    \endcode
*/

/*!
    \qmlsignal QtQuick::Dialogs::MessageDialog::accepted

    The \a accepted signal is emitted when the user has pressed the OK button
    on the dialog.

    Example:

    \qml
    MessageDialog {
        onAccepted: { console.log("accepted") }
    }
    \endqml
*/

/*!
    \qmlsignal QtQuick::Dialogs::MessageDialog::rejected

    The \a rejected signal is emitted when the user has dismissed the dialog,
    either by closing the dialog window or by pressing the Cancel button.
*/

/*!
    \class QQuickQMessageBox
    \inmodule QtQuick.PrivateWidgets
    \internal

    \brief The QQuickQMessageBox class is a wrapper for a QMessageBox.

    \since 5.2
*/

/*!
    Constructs a message dialog with parent window \a parent.
*/
QQuickQMessageBox::QQuickQMessageBox(QObject *parent)
    : QQuickAbstractMessageDialog(parent)
{
}

/*!
    Destroys the message dialog.
*/
QQuickQMessageBox::~QQuickQMessageBox()
{
    if (m_dlgHelper)
        m_dlgHelper->hide();
    delete m_dlgHelper;
}

void QQuickQMessageBox::finished(int button) {
    click(static_cast<StandardButton>(button));
}

void QQuickQMessageBox::clicked(QAbstractButton* button) {
    QMessageBox &mb = static_cast<QMessageBoxHelper*>(QQuickAbstractMessageDialog::m_dlgHelper)->m_dialog;
    switch (mb.buttonRole(button)) {
    case QMessageBox::AcceptRole:
        emit accepted();
        break;
    case QMessageBox::RejectRole:
        emit rejected();
        break;
    case QMessageBox::DestructiveRole:
        emit discard();
        break;
    case QMessageBox::HelpRole:
        emit help();
        break;
    case QMessageBox::YesRole:
        emit yes();
        break;
    case QMessageBox::NoRole:
        emit no();
        break;
    case QMessageBox::ApplyRole:
        emit apply();
        break;
    case QMessageBox::ResetRole:
        emit reset();
        break;
    default:
        qWarning("unhandled QMessageBox button role %d", mb.buttonRole(button));
    }
    if (!mb.isVisible())
        setVisible(false);
}

QPlatformDialogHelper *QQuickQMessageBox::helper()
{
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        m_parentWindow = parentItem->window();

    if (!QQuickAbstractMessageDialog::m_dlgHelper) {
        QMessageBoxHelper* helper = new QMessageBoxHelper();
        QQuickAbstractMessageDialog::m_dlgHelper = helper;
        connect(helper, SIGNAL(accept()), this, SLOT(accept()));
        connect(helper, SIGNAL(reject()), this, SLOT(reject()));
        connect(&helper->m_dialog, SIGNAL(finished(int)), this, SLOT(finished(int)));
        connect(&helper->m_dialog, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
    }

    return QQuickAbstractMessageDialog::m_dlgHelper;
}

QT_END_NAMESPACE
