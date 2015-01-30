/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#ifndef QQUICKABSTRACTSCROLLBAR_P_H
#define QQUICKABSTRACTSCROLLBAR_P_H

#include <QtQuickControls/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickFlickable;
class QQuickScrollBarAttached;
class QQuickAbstractScrollBarPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractScrollBar : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal size READ size NOTIFY sizeChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(QQuickItem *handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)

public:
    explicit QQuickAbstractScrollBar(QQuickItem *parent = Q_NULLPTR);

    static QQuickScrollBarAttached *qmlAttachedProperties(QObject *object);

    qreal size() const;
    qreal position() const;

    bool isActive() const;
    void setActive(bool active);

    bool isPressed() const;
    void setPressed(bool pressed);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    QQuickItem *handle() const;
    void setHandle(QQuickItem *handle);

public Q_SLOTS:
    void setSize(qreal size);
    void setPosition(qreal position);

Q_SIGNALS:
    void sizeChanged();
    void positionChanged();
    void activeChanged();
    void pressedChanged();
    void orientationChanged();
    void handleChanged();

protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    virtual qreal positionAt(const QPoint &point) const;

private:
    Q_DISABLE_COPY(QQuickAbstractScrollBar)
    Q_DECLARE_PRIVATE(QQuickAbstractScrollBar)
};

class QQuickScrollBarAttachedPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickScrollBarAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickAbstractScrollBar *horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged FINAL)
    Q_PROPERTY(QQuickAbstractScrollBar *vertical READ vertical WRITE setVertical NOTIFY verticalChanged FINAL)

public:
    explicit QQuickScrollBarAttached(QQuickFlickable *flickable);

    QQuickAbstractScrollBar *horizontal() const;
    void setHorizontal(QQuickAbstractScrollBar *horizontal);

    QQuickAbstractScrollBar *vertical() const;
    void setVertical(QQuickAbstractScrollBar *vertical);

Q_SIGNALS:
    void horizontalChanged();
    void verticalChanged();

private:
    Q_DISABLE_COPY(QQuickScrollBarAttached)
    Q_DECLARE_PRIVATE(QQuickScrollBarAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickAbstractScrollBar, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKABSTRACTSCROLLBAR_P_H
