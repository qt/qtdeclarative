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

#ifndef QAccessibleQuickView_H
#define QAccessibleQuickView_H

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

#include <QtGui/qaccessibleobject.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QAccessibleQuickWindow : public QAccessibleObject
{
public:
    QAccessibleQuickWindow(QQuickWindow *object);

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;
    QAccessibleInterface *focusChild() const override;

    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;

    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QString text(QAccessible::Text text) const override;
    QAccessibleInterface *childAt(int x, int y) const override;

private:
    QQuickWindow *window() const override { return static_cast<QQuickWindow*>(object()); }
    QList<QQuickItem *> rootItems() const;
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QAccessibleQuickView_H
