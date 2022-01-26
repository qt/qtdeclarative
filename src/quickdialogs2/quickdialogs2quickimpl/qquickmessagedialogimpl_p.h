/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKMESSAGEDIALOGIMPL_P_H
#define QQUICKMESSAGEDIALOGIMPL_P_H

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

#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickMessageDialogImplAttached;
class QQuickMessageDialogImplAttachedPrivate;
class QQuickMessageDialogImplPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickMessageDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY optionsChanged)
    Q_PROPERTY(QString informativeText READ informativeText NOTIFY optionsChanged)
    Q_PROPERTY(QString detailedText READ detailedText NOTIFY optionsChanged)
    Q_PROPERTY(bool showDetailedText READ showDetailedText NOTIFY showDetailedTextChanged)
    QML_NAMED_ELEMENT(MessageDialogImpl)
    QML_ATTACHED(QQuickMessageDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 3)
public:
    explicit QQuickMessageDialogImpl(QObject *parent = nullptr);

    static QQuickMessageDialogImplAttached *qmlAttachedProperties(QObject *object);

    QSharedPointer<QMessageDialogOptions> options() const;
    void setOptions(const QSharedPointer<QMessageDialogOptions> &options);

    bool showDetailedText() const;
    QString text() const;
    QString informativeText() const;
    QString detailedText() const;

Q_SIGNALS:
    void buttonClicked(QPlatformDialogHelper::StandardButton button,
                       QPlatformDialogHelper::ButtonRole role);
    void showDetailedTextChanged();
    void optionsChanged();

public Q_SLOTS:
    void toggleShowDetailedText();

private:
    Q_DISABLE_COPY(QQuickMessageDialogImpl)
    Q_DECLARE_PRIVATE(QQuickMessageDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickMessageDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox WRITE setButtonBox NOTIFY
                       buttonBoxChanged)
    Q_PROPERTY(QQuickButton *detailedTextButton READ detailedTextButton WRITE setDetailedTextButton
                       NOTIFY detailedTextButtonChanged)
public:
    explicit QQuickMessageDialogImplAttached(QObject *parent = nullptr);

    QQuickDialogButtonBox *buttonBox() const;
    void setButtonBox(QQuickDialogButtonBox *buttons);

    QQuickButton *detailedTextButton() const;
    void setDetailedTextButton(QQuickButton *detailedTextButton);

Q_SIGNALS:
    void buttonBoxChanged();
    void detailedTextButtonChanged();

private:
    Q_DISABLE_COPY(QQuickMessageDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickMessageDialogImplAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMessageDialogImpl)

#endif // QQUICKMESSAGEDIALOGIMPL_P_H
