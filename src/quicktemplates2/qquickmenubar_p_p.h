/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#ifndef QQUICKMENUBAR_P_P_H
#define QQUICKMENUBAR_P_P_H

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

#include <QtQuickTemplates2/private/qquickmenubar_p.h>
#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickMenuBarItem;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickMenuBarPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickMenuBar)

public:
    static QQuickMenuBarPrivate *get(QQuickMenuBar *menuBar)
    {
        return menuBar->d_func();
    }

    QQmlListProperty<QQuickMenu> menus();
    QQmlListProperty<QObject> contentData();

    QQuickItem *beginCreateItem(QQuickMenu *menu);
    void completeCreateItem();

    QQuickItem *createItem(QQuickMenu *menu);

    void toggleCurrentMenu(bool visible, bool activate);
    void activateItem(QQuickMenuBarItem *item);
    void activateNextItem();
    void activatePreviousItem();

    void onItemHovered();
    void onItemTriggered();
    void onMenuAboutToHide();

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);

    static void menus_append(QQmlListProperty<QQuickMenu> *prop, QQuickMenu *obj);
    static qsizetype menus_count(QQmlListProperty<QQuickMenu> *prop);
    static QQuickMenu *menus_at(QQmlListProperty<QQuickMenu> *prop, qsizetype index);
    static void menus_clear(QQmlListProperty<QQuickMenu> *prop);

    QPalette defaultPalette() const override;

    bool popupMode = false;
    bool triggering = false;
    QQmlComponent *delegate = nullptr;
    QPointer<QQuickMenuBarItem> currentItem;
};

QT_END_NAMESPACE

#endif // QQUICKMENUBAR_P_P_H
