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
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

class QQuickTextUtil : public QObject   // For the benefit of translations.
{
    Q_OBJECT
public:
    template <typename Private> static void setCursorDelegate(Private *d, QQmlComponent *delegate);
    template <typename Private> static void createCursor(Private *d);

    template <typename T> static typename T::RenderType textRenderType();

    static qreal alignedX(qreal textWidth, qreal itemWidth, int alignment);
    static qreal alignedY(qreal textHeight, qreal itemHeight, int alignment);

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

    Q_EMIT parent->cursorDelegateChanged();
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

template <typename T>
typename T::RenderType QQuickTextUtil::textRenderType()
{
    switch (QQuickWindow::textRenderType()) {
    case QQuickWindow::QtTextRendering:
        return T::QtRendering;
    case QQuickWindow::NativeTextRendering:
        return T::NativeRendering;
    }

    Q_UNREACHABLE();
    return T::QtRendering;
}

QT_END_NAMESPACE

#endif
