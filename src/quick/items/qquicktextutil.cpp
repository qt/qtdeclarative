// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktextutil_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcomponent.h>

#include <private/qqmlglobal_p.h>
#include <private/qquickitem_p.h>

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
            if (parent->clip())
                QQuickItemPrivate::get(parent)->dirty(QQuickItemPrivate::Size);
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
