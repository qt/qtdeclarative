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

#ifndef QQUICKCONTAINER_P_P_H
#define QQUICKCONTAINER_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <private/qqmlobjectmodel_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickContainerPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickContainer)

public:
    static QQuickContainerPrivate *get(QQuickContainer *container)
    {
        return container->d_func();
    }

    void init();
    void cleanup();

    QQuickItem *itemAt(int index) const;
    void insertItem(int index, QQuickItem *item);
    void moveItem(int from, int to, QQuickItem *item);
    void removeItem(int index, QQuickItem *item);
    void reorderItems();

    void _q_currentIndexChanged();

    void itemChildAdded(QQuickItem *item, QQuickItem *child) override;
    void itemSiblingOrderChanged(QQuickItem *item) override;
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) override;
    void itemDestroyed(QQuickItem *item) override;

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);
    static qsizetype contentData_count(QQmlListProperty<QObject> *prop);
    static QObject *contentData_at(QQmlListProperty<QObject> *prop, qsizetype index);
    static void contentData_clear(QQmlListProperty<QObject> *prop);

    static void contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *obj);
    static qsizetype contentChildren_count(QQmlListProperty<QQuickItem> *prop);
    static QQuickItem *contentChildren_at(QQmlListProperty<QQuickItem> *prop, qsizetype index);
    static void contentChildren_clear(QQmlListProperty<QQuickItem> *prop);

    void updateContentWidth();
    void updateContentHeight();

    bool hasContentWidth = false;
    bool hasContentHeight = false;
    qreal contentWidth = 0;
    qreal contentHeight = 0;
    QObjectList contentData;
    QQmlObjectModel *contentModel = nullptr;
    qsizetype currentIndex = -1;
    bool updatingCurrent = false;
    QQuickItemPrivate::ChangeTypes changeTypes = Destroyed | Parent | SiblingOrder;
};

QT_END_NAMESPACE

#endif // QQUICKCONTAINER_P_P_H
