/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktextutil_p.h"

#include <QtQml/qqmlinfo.h>

#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

QQuickItem *QQuickTextUtil::createCursor(
        QQmlComponent *component, QQuickItem *parent, const QRectF &rectangle, const char *className)
{
    QQuickItem *item = 0;
    if (component->isReady()) {
        QQmlContext *creationContext = component->creationContext();

        if (QObject *object = component->beginCreate(creationContext
                ? creationContext
                : qmlContext(parent))) {
            if ((item = qobject_cast<QQuickItem *>(object))) {
                QQml_setParent_noEvent(item, parent);
                item->setParentItem(parent);
                item->setPos(rectangle.topLeft());
                item->setHeight(rectangle.height());
            } else {
                qmlInfo(parent) << tr("%1 does not support loading non-visual cursor delegates.")
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
    qmlInfo(parent, component->errors()) << tr("Could not load cursor delegate");
    return item;
}

QT_END_NAMESPACE
