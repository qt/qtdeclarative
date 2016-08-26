/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKPOINTERHANDLER_H
#define QQUICKPOINTERHANDLER_H

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

#include "qevent.h"

#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPointerHandlerDispatch)

class Q_QUICK_PRIVATE_EXPORT QQuickPointerHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QQuickItem * target READ target WRITE setTarget NOTIFY targetChanged)

public:
    QQuickPointerHandler(QObject *parent = 0);
    virtual ~QQuickPointerHandler() { }

public:
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool active() const { return m_active; }

    QQuickItem *target() const { return m_target; }
    void setTarget(QQuickItem *target);

    void handlePointerEvent(QQuickPointerEvent *event);

Q_SIGNALS:
    void enabledChanged();
    void activeChanged();
    void targetChanged();
    void canceled(QQuickEventPoint *point);

protected:
    QQuickPointerEvent *currentEvent() { return m_currentEvent; }
    virtual bool wantsPointerEvent(QQuickPointerEvent *event);
    virtual void setActive(bool active);
    virtual void handlePointerEventImpl(QQuickPointerEvent *event);
    void setGrab(QQuickEventPoint *point, bool grab);
    virtual void handleGrabCancel(QQuickEventPoint *point);
    QPointF eventPos(const QQuickEventPoint *point) const;
    bool targetContains(const QQuickEventPoint *point) const;

private:
    QQuickPointerEvent *m_currentEvent;
    QQuickItem *m_target;
    bool m_enabled : 1;
    bool m_active : 1;

    friend class QQuickEventPoint;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPointerHandler)

#endif // QQUICKPOINTERHANDLER_H
