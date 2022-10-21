/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#ifndef QQUICKINDICATORBUTTON_H
#define QQUICKINDICATORBUTTON_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQml/qjsvalue.h>
#include "qquickdeferredpointer_p_p.h"

QT_BEGIN_NAMESPACE

class QQuickIndicatorButtonPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickIndicatorButton : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool hovered READ isHovered WRITE setHovered NOTIFY hoveredChanged FINAL REVISION(2, 1))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitIndicatorWidth READ implicitIndicatorWidth NOTIFY implicitIndicatorWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitIndicatorHeight READ implicitIndicatorHeight NOTIFY implicitIndicatorHeightChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "indicator")
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickIndicatorButton(QObject *parent);

    bool isPressed() const;
    void setPressed(bool pressed);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

    bool isHovered() const;
    void setHovered(bool hovered);

    qreal implicitIndicatorWidth() const;
    qreal implicitIndicatorHeight() const;

Q_SIGNALS:
    void pressedChanged();
    void indicatorChanged();
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void hoveredChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void implicitIndicatorWidthChanged();
    Q_REVISION(2, 5) void implicitIndicatorHeightChanged();

private:
    Q_DISABLE_COPY(QQuickIndicatorButton)
    Q_DECLARE_PRIVATE(QQuickIndicatorButton)
};

class QQuickIndicatorButtonPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickIndicatorButton)

public:
    static QQuickIndicatorButtonPrivate *get(QQuickIndicatorButton *button)
    {
        return button->d_func();
    }

    void cancelIndicator();
    void executeIndicator(bool complete = false);

    bool pressed = false;
    bool hovered = false;
    QQuickDeferredPointer<QQuickItem> indicator;
};

QT_END_NAMESPACE

#endif // QQUICKINDICATORBUTTON_H
