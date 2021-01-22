/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
**
**
**
****************************************************************************/

#include "qquicktextutil_p.h"

#include <QtQml/qqmlinfo.h>

#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

QQuickItem *QQuickTextUtil::createCursor(
        QQmlComponent *component, QQuickItem *parent, const QRectF &rectangle, const char *className)
{
    QQuickItem *item = nullptr;
    if (component->isReady()) {
        QQmlContext *creationContext = component->creationContext();

        if (QObject *object = component->beginCreate(creationContext
                ? creationContext
                : qmlContext(parent))) {
            if ((item = qobject_cast<QQuickItem *>(object))) {
                QQml_setParent_noEvent(item, parent);
                item->setParentItem(parent);
                item->setPosition(rectangle.topLeft());
                item->setHeight(rectangle.height());
            } else {
                qmlWarning(parent) << tr("%1 does not support loading non-visual cursor delegates.")
                        .arg(QString::fromUtf8(className));
            }
            component->completeCreate();
            return item;
        }
    } else if (component->isLoading()) {
        QObject::connect(component, SIGNAL(statusChanged(QQmlComponent::Status)),
                parent, SLOT(createCursor()), Qt::UniqueConnection);
        return item;
    }
    qmlWarning(parent, component->errors()) << tr("Could not load cursor delegate");
    return item;
}

qreal QQuickTextUtil::alignedX(const qreal textWidth, const qreal itemWidth, int alignment)
{
    qreal x = 0;
    switch (alignment) {
    case Qt::AlignLeft:
    case Qt::AlignJustify:
        break;
    case Qt::AlignRight:
        x = itemWidth - textWidth;
        break;
    case Qt::AlignHCenter:
        x = (itemWidth - textWidth) / 2;
        break;
    }
    return x;
}

qreal QQuickTextUtil::alignedY(const qreal textHeight, const qreal itemHeight, int alignment)
{
    qreal y = 0;
    switch (alignment) {
    case Qt::AlignTop:
        break;
    case Qt::AlignBottom:
        y = itemHeight - textHeight;
        break;
    case Qt::AlignVCenter:
        y = (itemHeight - textHeight) / 2;
        break;
    }
    return y;
}

QT_END_NAMESPACE

#include "moc_qquicktextutil_p.cpp"
