/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

#ifndef QQUICKSTYLE_P_H
#define QQUICKSTYLE_P_H

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

#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

namespace QQuickStyle
{
    template <typename T>
    static T *instance(QObject *object)
    {
        if (object)
            return qobject_cast<T*>(qmlAttachedPropertiesObject<T>(object, false));
        return Q_NULLPTR;
    }

    template <typename T>
    static T *findParent(QObject *object)
    {
        QQuickItem *item = qobject_cast<QQuickItem *>(object);
        if (item) {
            // lookup parent items
            QQuickItem *parent = item->parentItem();
            while (parent) {
                T *attached = instance<T>(parent);
                if (attached)
                    return attached;
                parent = parent->parentItem();
            }

            // fallback to item's window
            QQuickWindow *window = item->window();
            if (window) {
                T *attached = instance<T>(window);
                if (attached)
                    return attached;
            }
        }

        // lookup parent window
        QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
        if (window) {
            QQuickWindow *parentWindow = qobject_cast<QQuickWindow *>(window->parent());
            if (parentWindow) {
                T *attached = instance<T>(window);
                if (attached)
                    return attached;
            }
        }

        // fallback to engine (global)
        if (object) {
            QQmlEngine *engine = qmlEngine(object);
            if (engine) {
                QByteArray name = QByteArray("_q_") + T::staticMetaObject.className();
                T *instance = engine->property(name).value<T*>();
                if (!instance) {
                    instance = new T(engine);
                    engine->setProperty(name, QVariant::fromValue(instance));
                }
                return instance;
            }
        }

        return Q_NULLPTR;
    }

    template <typename T>
    static QList<T *> findChildren(QObject *object)
    {
        QList<T *> children;

        QQuickItem *item = qobject_cast<QQuickItem *>(object);
        if (!item) {
            QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
            if (window) {
                item = window->contentItem();

                foreach (QObject *child, window->children()) {
                    QQuickWindow *childWindow = qobject_cast<QQuickWindow *>(child);
                    if (childWindow) {
                        T *attached = instance<T>(childWindow);
                        if (attached)
                            children += attached;
                    }
                }
            }
        }

        if (item) {
            foreach (QQuickItem *child, item->childItems()) {
                T *attached = instance<T>(child);
                if (attached)
                    children += attached;
                else
                    children += findChildren<T>(child);
            }
        }

        return children;
    }
}

QT_END_NAMESPACE

#endif // QQUICKSTYLE_P_H
