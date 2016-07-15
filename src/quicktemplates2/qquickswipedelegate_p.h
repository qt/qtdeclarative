/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#ifndef QQUICKSWIPEDELEGATE_P_H
#define QQUICKSWIPEDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickSwipeDelegatePrivate;
class QQuickSwipe;
class QQuickSwipeDelegateAttached;
class QQuickSwipeDelegateAttachedPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSwipeDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(QQuickSwipe *swipe READ swipe CONSTANT)

public:
    explicit QQuickSwipeDelegate(QQuickItem *parent = nullptr);

    QQuickSwipe *swipe() const;

    static QQuickSwipeDelegateAttached *qmlAttachedProperties(QObject *object);

protected:
    bool childMouseEventFilter(QQuickItem *child, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QFont defaultFont() const override;

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickSwipeDelegate)
    Q_DECLARE_PRIVATE(QQuickSwipeDelegate)
};

class QQuickSwipePrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSwipe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(bool complete READ isComplete NOTIFY completeChanged FINAL)
    Q_PROPERTY(QQmlComponent *left READ left WRITE setLeft NOTIFY leftChanged FINAL)
    Q_PROPERTY(QQmlComponent *behind READ behind WRITE setBehind NOTIFY behindChanged FINAL)
    Q_PROPERTY(QQmlComponent *right READ right WRITE setRight NOTIFY rightChanged FINAL)
    Q_PROPERTY(QQuickItem *leftItem READ leftItem NOTIFY leftItemChanged FINAL)
    Q_PROPERTY(QQuickItem *behindItem READ behindItem NOTIFY behindItemChanged FINAL)
    Q_PROPERTY(QQuickItem *rightItem READ rightItem NOTIFY rightItemChanged FINAL)

public:
    explicit QQuickSwipe(QQuickSwipeDelegate *control);

    qreal position() const;
    void setPosition(qreal position);

    bool isComplete() const;
    void setComplete(bool complete);

    QQmlComponent *left() const;
    void setLeft(QQmlComponent *left);

    QQmlComponent *behind() const;
    void setBehind(QQmlComponent *behind);

    QQmlComponent *right() const;
    void setRight(QQmlComponent *right);

    QQuickItem *leftItem() const;
    void setLeftItem(QQuickItem *item);

    QQuickItem *behindItem() const;
    void setBehindItem(QQuickItem *item);

    QQuickItem *rightItem() const;
    void setRightItem(QQuickItem *item);

    Q_REVISION(1) Q_INVOKABLE void close();

Q_SIGNALS:
    void positionChanged();
    void completeChanged();
    Q_REVISION(1) void completed();
    void leftChanged();
    void behindChanged();
    void rightChanged();
    void leftItemChanged();
    void behindItemChanged();
    void rightItemChanged();

private:
    Q_DISABLE_COPY(QQuickSwipe)
    Q_DECLARE_PRIVATE(QQuickSwipe)
};

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSwipeDelegateAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)

public:
    explicit QQuickSwipeDelegateAttached(QObject *object = nullptr);

    bool isPressed() const;
    void setPressed(bool pressed);

Q_SIGNALS:
    void pressedChanged();
    void clicked();

private:
    Q_DISABLE_COPY(QQuickSwipeDelegateAttached)
    Q_DECLARE_PRIVATE(QQuickSwipeDelegateAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSwipeDelegate)
QML_DECLARE_TYPEINFO(QQuickSwipeDelegate, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKSWIPEDELEGATE_P_H
