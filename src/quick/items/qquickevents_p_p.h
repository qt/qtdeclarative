/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QQUICKEVENTS_P_P_H
#define QQUICKEVENTS_P_P_H

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

#include <private/qtquickglobal_p.h>
#include <qqml.h>

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtGui/qevent.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/qvector2d.h>
#include <QtQuick/qquickitem.h>

#if QT_CONFIG(shortcut)
#  include <QtGui/qkeysequence.h>
#endif

QT_BEGIN_NAMESPACE

class QPointingDevice;
class QPointerEvent;
class QMouseEvent;
class QQuickPointerHandler;

class QQuickKeyEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int key READ key CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(bool isAutoRepeat READ isAutoRepeat CONSTANT)
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(quint32 nativeScanCode READ nativeScanCode CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickKeyEvent()
    {}

    void reset(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
               const QString &text = QString(), bool autorep = false, ushort count = 1)
    {
        m_type = type;
        m_key = key;
        m_modifiers = modifiers;
        m_text = text;
        m_autoRepeat = autorep;
        m_count = count;
        setAccepted(false);
    }

    void reset(const QKeyEvent &ke)
    {
        m_type = ke.type();
        m_key = ke.key();
        m_modifiers = ke.modifiers();
        m_text = ke.text();
        m_autoRepeat = ke.isAutoRepeat();
        m_count = ke.count();
        m_nativeScanCode = ke.nativeScanCode();
        setAccepted(false);
    }

    int key() const { return m_key; }
    QString text() const { return m_text; }
    int modifiers() const { return m_modifiers; }
    bool isAutoRepeat() const { return m_autoRepeat; }
    int count() const { return m_count; }
    quint32 nativeScanCode() const { return m_nativeScanCode; }

    bool isAccepted() { return m_accepted; }
    void setAccepted(bool accepted) { m_accepted = accepted; }

#if QT_CONFIG(shortcut)
    Q_REVISION(2, 2) Q_INVOKABLE bool matches(QKeySequence::StandardKey key) const;
#endif

private:
    QString m_text;
    quint32 m_nativeScanCode;
    QEvent::Type m_type;
    int m_key;
    int m_modifiers;
    int m_count;
    bool m_accepted;
    bool m_autoRepeat;
};

// used in Qt Location
class Q_QUICK_PRIVATE_EXPORT QQuickMouseEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x CONSTANT)
    Q_PROPERTY(qreal y READ y CONSTANT)
    Q_PROPERTY(int button READ button CONSTANT)
    Q_PROPERTY(int buttons READ buttons CONSTANT)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(int source READ source CONSTANT REVISION(2, 7))
    Q_PROPERTY(bool wasHeld READ wasHeld CONSTANT)
    Q_PROPERTY(bool isClick READ isClick CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    Q_PROPERTY(int flags READ flags CONSTANT REVISION(2, 11))
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickMouseEvent()
      : _buttons(Qt::NoButton), _modifiers(Qt::NoModifier)
      , _wasHeld(false), _isClick(false), _accepted(false)
    {}

    void reset(qreal x, qreal y, Qt::MouseButton button, Qt::MouseButtons buttons,
               Qt::KeyboardModifiers modifiers, bool isClick = false, bool wasHeld = false,
               Qt::MouseEventFlags flags = { })
    {
        _x = x;
        _y = y;
        _button = button;
        _buttons = buttons;
        _modifiers = modifiers;
        _source = Qt::MouseEventNotSynthesized;
        _wasHeld = wasHeld;
        _isClick = isClick;
        _accepted = true;
        _flags = flags;
    }

    qreal x() const { return _x; }
    qreal y() const { return _y; }
    int button() const { return _button; }
    int buttons() const { return _buttons; }
    int modifiers() const { return _modifiers; }
    int source() const { return _source; }
    bool wasHeld() const { return _wasHeld; }
    bool isClick() const { return _isClick; }

    // only for internal usage
    void setX(qreal x) { _x = x; }
    void setY(qreal y) { _y = y; }
    void setPosition(const QPointF &point) { _x = point.x(); _y = point.y(); }
    void setSource(Qt::MouseEventSource s) { _source = s; }

    bool isAccepted() { return _accepted; }
    void setAccepted(bool accepted) { _accepted = accepted; }
    int flags() const { return _flags; }
private:
    qreal _x = 0;
    qreal _y = 0;
    Qt::MouseButton _button = Qt::NoButton;
    Qt::MouseButtons _buttons;
    Qt::KeyboardModifiers _modifiers;
    Qt::MouseEventSource _source = Qt::MouseEventNotSynthesized;
    bool _wasHeld : 1;
    bool _isClick : 1;
    bool _accepted : 1;
    Qt::MouseEventFlags _flags;
};

class QQuickWheelEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(const QPointingDevice *device READ pointingDevice CONSTANT)
    Q_PROPERTY(qreal x READ x CONSTANT)
    Q_PROPERTY(qreal y READ y CONSTANT)
    Q_PROPERTY(QPoint angleDelta READ angleDelta CONSTANT)
    Q_PROPERTY(QPoint pixelDelta READ pixelDelta CONSTANT)
    Q_PROPERTY(Qt::ScrollPhase phase READ phase CONSTANT)
    Q_PROPERTY(int buttons READ buttons CONSTANT)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(bool inverted READ inverted CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickWheelEvent() = default;

    void reset(const QWheelEvent *event)
    {
        _device = event->pointingDevice();
        _x = event->position().x();
        _y = event->position().y();
        _angleDelta = event->angleDelta();
        _pixelDelta = event->pixelDelta();
        _buttons = event->buttons();
        _modifiers = event->modifiers();
        _accepted = true;
        _inverted = event->inverted();
        _phase = event->phase();
    }

    const QPointingDevice *pointingDevice() const { return _device; }
    qreal x() const { return _x; }
    qreal y() const { return _y; }
    QPoint angleDelta() const { return _angleDelta; }
    QPoint pixelDelta() const { return _pixelDelta; }
    int buttons() const { return _buttons; }
    int modifiers() const { return _modifiers; }
    Qt::ScrollPhase phase() const { return _phase; }
    bool inverted() const { return _inverted; }
    bool isAccepted() { return _accepted; }
    void setAccepted(bool accepted) { _accepted = accepted; }

private:
    const QPointingDevice *_device = nullptr;
    qreal _x = 0;
    qreal _y = 0;
    QPoint _angleDelta;
    QPoint _pixelDelta;
    Qt::MouseButtons _buttons = Qt::NoButton;
    Qt::KeyboardModifiers _modifiers = Qt::NoModifier;
    Qt::ScrollPhase _phase = Qt::NoScrollPhase;
    bool _inverted = false;
    bool _accepted = false;
};

class Q_QUICK_PRIVATE_EXPORT QQuickCloseEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickCloseEvent() {}

    bool isAccepted() { return _accepted; }
    void setAccepted(bool accepted) { _accepted = accepted; }

private:
    bool _accepted = true;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickKeyEvent)
QML_DECLARE_TYPE(QQuickMouseEvent)
QML_DECLARE_TYPE(QQuickWheelEvent)
QML_DECLARE_TYPE(QQuickCloseEvent)
QML_DECLARE_TYPE(QPointingDevice)
QML_DECLARE_TYPE(QPointingDeviceUniqueId)
QML_DECLARE_TYPE(QPointerEvent)

#endif // QQUICKEVENTS_P_P_H
