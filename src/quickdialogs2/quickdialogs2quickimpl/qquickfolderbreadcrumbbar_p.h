/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKFOLDERBREADCRUMBBAR_P_H
#define QQUICKFOLDERBREADCRUMBBAR_P_H

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

#include <QtQml/qqmlcomponent.h>
#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickFolderBreadcrumbBarPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFolderBreadcrumbBar : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(QQuickFileDialogImpl *fileDialog READ fileDialog WRITE setFileDialog NOTIFY fileDialogChanged)
    Q_PROPERTY(QQmlComponent *buttonDelegate READ buttonDelegate WRITE setButtonDelegate NOTIFY buttonDelegateChanged)
    Q_PROPERTY(QQmlComponent *separatorDelegate READ separatorDelegate WRITE setSeparatorDelegate NOTIFY separatorDelegateChanged)
    Q_PROPERTY(QQuickAbstractButton *upButton READ upButton WRITE setUpButton NOTIFY upButtonChanged)
    Q_PROPERTY(QQuickTextField *textField READ textField WRITE setTextField NOTIFY textFieldChanged)
    Q_PROPERTY(int upButtonSpacing READ upButtonSpacing WRITE setUpButtonSpacing NOTIFY upButtonSpacingChanged)
    QML_NAMED_ELEMENT(FolderBreadcrumbBar)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFolderBreadcrumbBar(QQuickItem *parent = nullptr);

    QQuickFileDialogImpl *fileDialog() const;
    void setFileDialog(QQuickFileDialogImpl *fileDialog);

    QQmlComponent *buttonDelegate();
    void setButtonDelegate(QQmlComponent *delegate);

    QQmlComponent *separatorDelegate();
    void setSeparatorDelegate(QQmlComponent *delegate);

    QQuickAbstractButton *upButton();
    void setUpButton(QQuickAbstractButton *upButton);

    int upButtonSpacing() const;
    void setUpButtonSpacing(int upButtonSpacing);

    QQuickTextField *textField();
    void setTextField(QQuickTextField *textField);

Q_SIGNALS:
    void fileDialogChanged();
    void buttonDelegateChanged();
    void separatorDelegateChanged();
    void upButtonChanged();
    void upButtonSpacingChanged();
    void textFieldChanged();

protected:
    bool event(QEvent *event) override;

    void componentComplete() override;

    void itemChange(ItemChange change, const ItemChangeData &data) override;

    bool isContent(QQuickItem *item) const override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickFolderBreadcrumbBar)
    Q_DECLARE_PRIVATE(QQuickFolderBreadcrumbBar)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFolderBreadcrumbBar)

#endif // QQUICKFOLDERBREADCRUMBBAR_P_H
