/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQUICKSELECTIONRECTANGLE_P_H
#define QQUICKSELECTIONRECTANGLE_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickSelectionRectanglePrivate;
class QQuickSelectable;
class QQuickSelectionRectangleAttached;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSelectionRectangle : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged FINAL)
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    Q_PROPERTY(QQmlComponent *topLeftHandle READ topLeftHandle WRITE setTopLeftHandle NOTIFY topLeftHandleChanged FINAL)
    Q_PROPERTY(QQmlComponent *bottomRightHandle READ bottomRightHandle WRITE setBottomRightHandle NOTIFY bottomRightHandleChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged FINAL)

    QML_NAMED_ELEMENT(SelectionRectangle)
    QML_ATTACHED(QQuickSelectionRectangleAttached)
    QML_ADDED_IN_VERSION(6, 2)

public:
    enum SelectionMode {
        Drag,
        PressAndHold,
        Auto
    };
    Q_ENUM(SelectionMode)

    explicit QQuickSelectionRectangle(QQuickItem *parent = nullptr);

    QQuickItem *target() const;
    void setTarget(QQuickItem *target);

    bool active();
    bool dragging();

    SelectionMode selectionMode() const;
    void setSelectionMode(SelectionMode selectionMode);

    QQmlComponent *topLeftHandle() const;
    void setTopLeftHandle(QQmlComponent *topLeftHandle);
    QQmlComponent *bottomRightHandle() const;
    void setBottomRightHandle(QQmlComponent *bottomRightHandle);

    static QQuickSelectionRectangleAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void targetChanged();
    void activeChanged();
    void draggingChanged();
    void topLeftHandleChanged();
    void bottomRightHandleChanged();
    void selectionModeChanged();

private:
    Q_DISABLE_COPY(QQuickSelectionRectangle)
    Q_DECLARE_PRIVATE(QQuickSelectionRectangle)
};

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSelectionRectangleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickSelectionRectangle *control READ control NOTIFY controlChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged FINAL)

public:
    QQuickSelectionRectangleAttached(QObject *parent);

    QQuickSelectionRectangle *control() const;
    void setControl(QQuickSelectionRectangle *control);

    bool dragging() const;
    void setDragging(bool dragging);

Q_SIGNALS:
    void controlChanged();
    void draggingChanged();

private:
    QPointer<QQuickSelectionRectangle> m_control;
    bool m_dragging = false;

    friend class QQuickSelectionRectanglePrivate;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSelectionRectangle)

#endif // QQUICKSELECTIONRECTANGLE_P_H
