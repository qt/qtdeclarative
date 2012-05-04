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

#ifndef QQUICKTEXTUTIL_P_H
#define QQUICKTEXTUTIL_P_H

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
#include <QtQml/qqmlincubator.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQuickTextUtil : public QObject   // For the benefit of translations.
{
    Q_OBJECT
public:
    template <typename Private> static void setCursorDelegate(Private *d, QQmlComponent *delegate);
    template <typename Private> static void createCursor(Private *d);

private:
    static QQuickItem *createCursor(
            QQmlComponent *component,
            QQuickItem *parent,
            const QRectF &cursorRectangle,
            const char *className);
};

template <typename Private>
void QQuickTextUtil::setCursorDelegate(Private *d, QQmlComponent *delegate)
{
    if (d->cursorComponent == delegate)
        return;

    typename Private::Public *parent = d->q_func();

    if (d->cursorComponent) {
        disconnect(d->cursorComponent, SIGNAL(statusChanged(QQmlComponent::Status)),
                parent, SLOT(createCursor()));
    }

    delete d->cursorItem;
    d->cursorItem = 0;
    d->cursorPending = true;

    d->cursorComponent = delegate;

    if (parent->isCursorVisible() && parent->isComponentComplete())
        createCursor(d);

    emit parent->cursorDelegateChanged();
}

template <typename Private>
void QQuickTextUtil::createCursor(Private *d)
{
    if (!d->cursorPending)
        return;

    d->cursorPending = false;

    typename Private::Public *parent = d->q_func();
    if (d->cursorComponent) {
        d->cursorItem = createCursor(
                d->cursorComponent,
                parent,
                parent->cursorRectangle(),
                Private::Public::staticMetaObject.className());
    }

    d->setNativeCursorEnabled(!d->cursorItem);
    d->updateType = Private::UpdatePaintNode;
    parent->update();
}

QT_END_NAMESPACE

#endif
