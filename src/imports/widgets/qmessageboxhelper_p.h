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

#ifndef QMESSAGEBOXHELPER_P_H
#define QMESSAGEBOXHELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QMessageBox>
#include "../dialogs/qquickabstractmessagedialog_p.h"

QT_BEGIN_NAMESPACE

class QMessageBoxHelper : public QPlatformMessageDialogHelper
{
    Q_OBJECT
public:
    QMessageBoxHelper() {
        connect(&m_dialog, SIGNAL(accepted()), this, SIGNAL(accept()));
        connect(&m_dialog, SIGNAL(rejected()), this, SIGNAL(reject()));
        connect(&m_dialog, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
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

public Q_SLOTS:
    void buttonClicked(QAbstractButton* button) {
        emit clicked(static_cast<QMessageDialogOptions::StandardButton>(m_dialog.standardButton(button)),
            static_cast<QMessageDialogOptions::ButtonRole>(m_dialog.buttonRole(button)));
    }
};

QT_END_NAMESPACE

#endif // QMESSAGEBOXHELPER_P_H
