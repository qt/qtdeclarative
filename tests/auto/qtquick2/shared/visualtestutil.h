/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef QQUICKVISUALTESTUTIL_H
#define QQUICKVISUALTESTUTIL_H

#include <QtQuick/QQuickItem>
#include <QtDeclarative/QDeclarativeExpression>

namespace QQuickVisualTestUtil
{
    QQuickItem *findVisibleChild(QQuickItem *parent, const QString &objectName);

    void dumpTree(QQuickItem *parent, int depth = 0);


    /*
       Find an item with the specified objectName.  If index is supplied then the
       item must also evaluate the {index} expression equal to index
    */
    template<typename T>
    T *findItem(QQuickItem *parent, const QString &objectName, int index = -1)
    {
        const QMetaObject &mo = T::staticMetaObject;
        for (int i = 0; i < parent->childItems().count(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
            if (!item)
                continue;
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
                if (index != -1) {
                    QDeclarativeExpression e(qmlContext(item), item, "index");
                    if (e.evaluate().toInt() == index)
                        return static_cast<T*>(item);
                } else {
                    return static_cast<T*>(item);
                }
            }
            item = findItem<T>(item, objectName, index);
            if (item)
                return static_cast<T*>(item);
        }

        return 0;
    }

    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName, bool visibleOnly = true)
    {
        QList<T*> items;
        const QMetaObject &mo = T::staticMetaObject;
        for (int i = 0; i < parent->childItems().count(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
            if (!item || (visibleOnly && !item->isVisible()))
                continue;
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
                items.append(static_cast<T*>(item));
            items += findItems<T>(item, objectName);
        }

        return items;
    }

    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName, const QList<int> &indexes)
    {
        QList<T*> items;
        for (int i=0; i<indexes.count(); i++)
            items << qobject_cast<QQuickItem*>(findItem<T>(parent, objectName, indexes[i]));
        return items;
    }

}

#endif // QQUICKVISUALTESTUTIL_H
