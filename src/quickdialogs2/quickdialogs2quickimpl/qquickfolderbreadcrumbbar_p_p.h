/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICKFOLDERBREADCRUMBBAR_P_P_H
#define QQUICKFOLDERBREADCRUMBBAR_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

#include "qquickfiledialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickAbstractButton;
class QQuickTextField;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFolderBreadcrumbBarPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickFolderBreadcrumbBar)

public:
    QQuickItem *createDelegateItem(QQmlComponent *component, const QVariantMap &initialProperties);
    static QString folderBaseName(const QString &folderPath);
    static QStringList crumbPathsForFolder(const QUrl &folder);
    void repopulate();
    void crumbClicked();
    void folderChanged();

    void cancelUpButton();
    void executeUpButton(bool complete = false);
    void goUp();

    void cancelTextField();
    void executeTextField(bool complete = false);
    void toggleTextFieldVisibility();
    void textFieldAccepted();

    void textFieldVisibleChanged();
    void textFieldActiveFocusChanged();
    void handleTextFieldShown();
    void handleTextFieldHidden();
    void ungrabEditPathShortcuts();

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;
    void resizeContent() override;

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

private:
    QQuickFileDialogImpl *fileDialog = nullptr;
    QList<QString> folderPaths;
    QQmlComponent *buttonDelegate = nullptr;
    QQmlComponent *separatorDelegate = nullptr;
    QQuickDeferredPointer<QQuickAbstractButton> upButton;
    QQuickDeferredPointer<QQuickTextField> textField;
    int editPathToggleShortcutId = 0;
    int editPathBackShortcutId = 0;
    int editPathEscapeShortcutId = 0;
    int goUpShortcutId = 0;
    int upButtonSpacing = 0;
    bool repopulating = false;
};

QT_END_NAMESPACE

#endif // QQUICKFOLDERBREADCRUMBBAR_P_P_H
