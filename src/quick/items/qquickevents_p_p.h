/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include <QtGui/qvector2d.h>
#include <QtGui/qevent.h>
#include <QtGui/qkeysequence.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQuickPointerDevice;
class QQuickPointerEvent;
class QQuickPointerMouseEvent;
#if QT_CONFIG(gestures)
class QQuickPointerNativeGestureEvent;
#endif
class QQuickPointerTabletEvent;
class QQuickPointerTouchEvent;
class QQuickPointerHandler;

class QQuickKeyEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int key READ key)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(int modifiers READ modifiers)
    Q_PROPERTY(bool isAutoRepeat READ isAutoRepeat)
    Q_PROPERTY(int count READ count)
    Q_PROPERTY(quint32 nativeScanCode READ nativeScanCode)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)

public:
    QQuickKeyEvent()
        : event(QEvent::None, 0, nullptr)
    {}

    void reset(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
               const QString &text = QString(), bool autorep = false, ushort count = 1)
    {
        event = QKeyEvent(type, key, modifiers, text, autorep, count);
        event.setAccepted(false);
    }

    void reset(const QKeyEvent &ke)
    {
        event = ke;
        event.setAccepted(false);
    }

    int key() const { return event.key(); }
    QString text() const { return event.text(); }
    int modifiers() const { return event.modifiers(); }
    bool isAutoRepeat() const { return event.isAutoRepeat(); }
    int count() const { return event.count(); }
    quint32 nativeScanCode() const { return event.nativeScanCode(); }

    bool isAccepted() { return event.isAccepted(); }
    void setAccepted(bool accepted) { event.setAccepted(accepted); }

#if QT_CONFIG(shortcut)
    Q_REVISION(2) Q_INVOKABLE bool matches(QKeySequence::StandardKey key) const { return event.matches(key); }
#endif

private:
    QKeyEvent event;
};

// used in Qt Location
class Q_QUICK_PRIVATE_EXPORT QQuickMouseEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x)
    Q_PROPERTY(qreal y READ y)
    Q_PROPERTY(int button READ button)
    Q_PROPERTY(int buttons READ buttons)
    Q_PROPERTY(int modifiers READ modifiers)
    Q_PROPERTY(int source READ source REVISION 7)
    Q_PROPERTY(bool wasHeld READ wasHeld)
    Q_PROPERTY(bool isClick READ isClick)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    Q_REVISION(11) Q_PROPERTY(int flags READ flags)

public:
    QQuickMouseEvent()
      : _buttons(Qt::NoButton), _modifiers(Qt::NoModifier)
      , _wasHeld(false), _isClick(false), _accepted(false)
      , _flags(Qt::MouseEventFlags(nullptr))
    {}

    void reset(qreal x, qreal y, Qt::MouseButton button, Qt::MouseButtons buttons,
               Qt::KeyboardModifiers modifiers, bool isClick = false, bool wasHeld = false,
               Qt::MouseEventFlags flags = nullptr)
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
    Q_PROPERTY(qreal x READ x)
    Q_PROPERTY(qreal y READ y)
    Q_PROPERTY(QPoint angleDelta READ angleDelta)
    Q_PROPERTY(QPoint pixelDelta READ pixelDelta)
    Q_PROPERTY(int buttons READ buttons)
    Q_PROPERTY(int modifiers READ modifiers)
    Q_PROPERTY(bool inverted READ inverted)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)

public:
    QQuickWheelEvent()
      : _buttons(Qt::NoButton), _modifiers(Qt::NoModifier)
    {}

    void reset(qreal x, qreal y, const QPoint &angleDelta, const QPoint &pixelDelta,
                     Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, bool inverted)
    {
        _x = x;
        _y = y;
        _angleDelta = angleDelta;
        _pixelDelta = pixelDelta;
        _buttons = buttons;
        _modifiers = modifiers;
        _accepted = true;
        _inverted = inverted;
    }

    qreal x() const { return _x; }
    qreal y() const { return _y; }
    QPoint angleDelta() const { return _angleDelta; }
    QPoint pixelDelta() const { return _pixelDelta; }
    int buttons() const { return _buttons; }
    int modifiers() const { return _modifiers; }
    bool inverted() const { return _inverted; }
    bool isAccepted() { return _accepted; }
    void setAccepted(bool accepted) { _accepted = accepted; }

private:
    qreal _x = 0;
    qreal _y = 0;
    QPoint _angleDelta;
    QPoint _pixelDelta;
    Qt::MouseButtons _buttons;
    Qt::KeyboardModifiers _modifiers;
    bool _inverted = false;
    bool _accepted = false;
};

class Q_QUICK_PRIVATE_EXPORT QQuickCloseEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)

public:
    QQuickCloseEvent() {}

    bool isAccepted() { return _accepted; }
    void setAccepted(bool accepted) { _accepted = accepted; }

private:
    bool _accepted = true;
};

class Q_QUICK_PRIVATE_EXPORT QQuickEventPoint : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickPointerEvent *event READ pointerEvent)
    Q_PROPERTY(QPointF position READ position)
    Q_PROPERTY(QPointF scenePosition READ scenePosition)
    Q_PROPERTY(QPointF scenePressPosition READ scenePressPosition)
    Q_PROPERTY(QPointF sceneGrabPosition READ sceneGrabPosition)
    Q_PROPERTY(State state READ state)
    Q_PROPERTY(int pointId READ pointId)
    Q_PROPERTY(qreal timeHeld READ timeHeld)
    Q_PROPERTY(QVector2D velocity READ velocity)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    Q_PROPERTY(QObject *exclusiveGrabber READ exclusiveGrabber WRITE setExclusiveGrabber)

public:
    enum State {
        Pressed     = Qt::TouchPointPressed,
        Updated     = Qt::TouchPointMoved,
        Stationary  = Qt::TouchPointStationary,
        Released    = Qt::TouchPointReleased
    };
    Q_DECLARE_FLAGS(States, State)
    Q_FLAG(States)

    enum GrabState {
        GrabPassive = 0x01,
        UngrabPassive = 0x02,
        CancelGrabPassive = 0x03,
        OverrideGrabPassive = 0x04,
        GrabExclusive = 0x10,
        UngrabExclusive = 0x20,
        CancelGrabExclusive = 0x30,
    };
    Q_ENUM(GrabState)

    QQuickEventPoint(QQuickPointerEvent *parent);

    void reset(Qt::TouchPointState state, const QPointF &scenePosition, int pointId, ulong timestamp, const QVector2D &velocity = QVector2D());
    void localizePosition(QQuickItem *target);

    QQuickPointerEvent *pointerEvent() const;
    QPointF position() const { return m_pos; }
    QPointF scenePosition() const { return m_scenePos; }
    QPointF scenePressPosition() const { return m_scenePressPos; }
    QPointF sceneGrabPosition() const { return m_sceneGrabPos; }
    QVector2D velocity() const { return m_velocity; }
    State state() const { return m_state; }
    int pointId() const { return m_pointId; }
    qreal timeHeld() const { return (m_timestamp - m_pressTimestamp) / 1000.0; }
    bool isAccepted() const { return m_accept; }
    void setAccepted(bool accepted = true);
    QObject *exclusiveGrabber() const;
    void setExclusiveGrabber(QObject *exclusiveGrabber);

    QQuickItem *grabberItem() const;
    void setGrabberItem(QQuickItem *exclusiveGrabber);

    QQuickPointerHandler *grabberPointerHandler() const;
    void setGrabberPointerHandler(QQuickPointerHandler *exclusiveGrabber, bool exclusive = false);

    void cancelExclusiveGrab();
    void cancelPassiveGrab(QQuickPointerHandler *handler);
    bool removePassiveGrabber(QQuickPointerHandler *handler);
    void cancelAllGrabs(QQuickPointerHandler *handler);

    QVector<QPointer <QQuickPointerHandler> > passiveGrabbers() const { return m_passiveGrabbers; }
    void setPassiveGrabbers(const QVector<QPointer <QQuickPointerHandler> > &grabbers) { m_passiveGrabbers = grabbers; }
    void clearPassiveGrabbers() { m_passiveGrabbers.clear(); }

protected:
    void cancelExclusiveGrabImpl(QTouchEvent *cancelEvent = nullptr);

private:
    QVector2D estimatedVelocity() const;

protected:
    QPointF m_pos;
    QPointF m_scenePos;
    QPointF m_scenePressPos;
    QPointF m_sceneGrabPos;
    QVector2D m_velocity;
    int m_pointId;
    QPointer<QObject> m_exclusiveGrabber;
    QVector<QPointer <QQuickPointerHandler> > m_passiveGrabbers;
    ulong m_timestamp;
    ulong m_pressTimestamp;
    State m_state;
    bool m_accept : 1;
    bool m_grabberIsHandler : 1;
    int m_reserved : 29;

    friend class QQuickPointerTouchEvent;
    friend class QQuickWindowPrivate;

    Q_DISABLE_COPY(QQuickEventPoint)
};

class Q_QUICK_PRIVATE_EXPORT QQuickEventTouchPoint : public QQuickEventPoint
{
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation)
    Q_PROPERTY(qreal pressure READ pressure)
    Q_PROPERTY(QSizeF ellipseDiameters READ ellipseDiameters)
    Q_PROPERTY(QPointingDeviceUniqueId uniqueId READ uniqueId)

public:
    QQuickEventTouchPoint(QQuickPointerTouchEvent *parent);

    void reset(const QTouchEvent::TouchPoint &tp, ulong timestamp);

    qreal rotation() const { return m_rotation; }
    qreal pressure() const { return m_pressure; }
    QSizeF ellipseDiameters() const { return m_ellipseDiameters; }
    QPointingDeviceUniqueId uniqueId() const { return m_uniqueId; }

private:
    qreal m_rotation;
    qreal m_pressure;
    QSizeF m_ellipseDiameters;
    QPointingDeviceUniqueId m_uniqueId;

    friend class QQuickPointerTouchEvent;

    Q_DISABLE_COPY(QQuickEventTouchPoint)
};

class Q_QUICK_PRIVATE_EXPORT QQuickPointerEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickPointerDevice *device READ device)
    Q_PROPERTY(Qt::KeyboardModifiers modifiers READ modifiers)
    Q_PROPERTY(Qt::MouseButtons button READ button)
    Q_PROPERTY(Qt::MouseButtons buttons READ buttons)

public:
    QQuickPointerEvent(QObject *parent = nullptr, QQuickPointerDevice *device = nullptr)
      : QObject(parent)
      , m_device(device)
      , m_pressedButtons(Qt::NoButton)
    {}

    ~QQuickPointerEvent() override;

public: // property accessors
    QQuickPointerDevice *device() const { return m_device; }
    Qt::KeyboardModifiers modifiers() const { return m_event ? m_event->modifiers() : Qt::NoModifier; }
    Qt::MouseButton button() const { return m_button; }
    Qt::MouseButtons buttons() const { return m_pressedButtons; }

public: // helpers for C++ only (during event delivery)
    virtual QQuickPointerEvent *reset(QEvent *ev) = 0;
    virtual void localize(QQuickItem *target) = 0;

    virtual bool isPressEvent() const = 0;
    virtual bool isDoubleClickEvent() const { return false; }
    virtual bool isUpdateEvent() const = 0;
    virtual bool isReleaseEvent() const = 0;
    virtual QQuickPointerMouseEvent *asPointerMouseEvent() { return nullptr; }
    virtual QQuickPointerTouchEvent *asPointerTouchEvent() { return nullptr; }
    virtual QQuickPointerTabletEvent *asPointerTabletEvent() { return nullptr; }
#if QT_CONFIG(gestures)
    virtual QQuickPointerNativeGestureEvent *asPointerNativeGestureEvent() { return nullptr; }
#endif
    virtual const QQuickPointerMouseEvent *asPointerMouseEvent() const { return nullptr; }
    virtual const QQuickPointerTouchEvent *asPointerTouchEvent() const { return nullptr; }
    virtual const QQuickPointerTabletEvent *asPointerTabletEvent() const { return nullptr; }
#if QT_CONFIG(gestures)
    virtual const QQuickPointerNativeGestureEvent *asPointerNativeGestureEvent() const { return nullptr; }
#endif
    virtual bool allPointsAccepted() const = 0;
    virtual bool allUpdatedPointsAccepted() const = 0;
    virtual bool allPointsGrabbed() const = 0;
    bool isAccepted() { return m_event->isAccepted(); }
    void setAccepted(bool accepted) { if (m_event) m_event->setAccepted(accepted); }
    QVector<QPointF> unacceptedPressedPointScenePositions() const;

    virtual int pointCount() const = 0;
    virtual QQuickEventPoint *point(int i) const = 0;
    virtual QQuickEventPoint *pointById(int pointId) const = 0;
    virtual QVector<QObject *> exclusiveGrabbers() const = 0;
    virtual void clearGrabbers() const = 0;
    virtual bool hasExclusiveGrabber(const QQuickPointerHandler *handler) const = 0;

    ulong timestamp() const { return m_event->timestamp(); }

protected:
    QQuickPointerDevice *m_device;
    QInputEvent *m_event = nullptr; // original event as received by QQuickWindow
    Qt::MouseButton m_button = Qt::NoButton;
    Qt::MouseButtons m_pressedButtons;

    Q_DISABLE_COPY(QQuickPointerEvent)
};

class Q_QUICK_PRIVATE_EXPORT QQuickPointerMouseEvent : public QQuickPointerEvent
{
    Q_OBJECT
public:
    QQuickPointerMouseEvent(QObject *parent = nullptr, QQuickPointerDevice *device = nullptr)
        : QQuickPointerEvent(parent, device), m_mousePoint(new QQuickEventPoint(this)) { }

    QQuickPointerEvent *reset(QEvent *) override;
    void localize(QQuickItem *target) override;
    bool isPressEvent() const override;
    bool isDoubleClickEvent() const override;
    bool isUpdateEvent() const override;
    bool isReleaseEvent() const override;
    QQuickPointerMouseEvent *asPointerMouseEvent() override { return this; }
    const QQuickPointerMouseEvent *asPointerMouseEvent() const override { return this; }
    int pointCount() const override { return 1; }
    QQuickEventPoint *point(int i) const override;
    QQuickEventPoint *pointById(int pointId) const override;
    bool allPointsAccepted() const override;
    bool allUpdatedPointsAccepted() const override;
    bool allPointsGrabbed() const override;
    QVector<QObject *> exclusiveGrabbers() const override;
    void clearGrabbers() const override;
    bool hasExclusiveGrabber(const QQuickPointerHandler *handler) const override;

    QMouseEvent *asMouseEvent(const QPointF& localPos) const;

private:
    QQuickEventPoint *m_mousePoint;

    Q_DISABLE_COPY(QQuickPointerMouseEvent)
};

class Q_QUICK_PRIVATE_EXPORT QQuickPointerTouchEvent : public QQuickPointerEvent
{
    Q_OBJECT
public:
    QQuickPointerTouchEvent(QObject *parent = nullptr, QQuickPointerDevice *device = nullptr)
        : QQuickPointerEvent(parent, device)
        , m_synthMouseEvent(QEvent::MouseMove, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier)
    {}

    QQuickPointerEvent *reset(QEvent *) override;
    void localize(QQuickItem *target) override;
    bool isPressEvent() const override;
    bool isUpdateEvent() const override;
    bool isReleaseEvent() const override;
    QQuickPointerTouchEvent *asPointerTouchEvent() override { return this; }
    const QQuickPointerTouchEvent *asPointerTouchEvent() const override { return this; }
    int pointCount() const override { return m_pointCount; }
    QQuickEventPoint *point(int i) const override;
    QQuickEventPoint *pointById(int pointId) const override;
    const QTouchEvent::TouchPoint *touchPointById(int pointId) const;
    bool allPointsAccepted() const override;
    bool allUpdatedPointsAccepted() const override;
    bool allPointsGrabbed() const override;
    QVector<QObject *> exclusiveGrabbers() const override;
    void clearGrabbers() const override;
    bool hasExclusiveGrabber(const QQuickPointerHandler *handler) const override;

    QMouseEvent *syntheticMouseEvent(int pointID, QQuickItem *relativeTo) const;
    QTouchEvent *touchEventForItem(QQuickItem *item, bool isFiltering = false) const;

    QTouchEvent *asTouchEvent() const;

private:
    Qt::TouchPointStates touchPointStates() const;

    int m_pointCount = 0;
    QVector<QQuickEventTouchPoint *> m_touchPoints;
    mutable QMouseEvent m_synthMouseEvent;

    Q_DISABLE_COPY(QQuickPointerTouchEvent)
};

#if QT_CONFIG(gestures)
class Q_QUICK_PRIVATE_EXPORT QQuickPointerNativeGestureEvent : public QQuickPointerEvent
{
    Q_OBJECT
    Q_PROPERTY(Qt::NativeGestureType type READ type CONSTANT)
    Q_PROPERTY(qreal value READ value CONSTANT)

public:
    QQuickPointerNativeGestureEvent(QObject *parent = nullptr, QQuickPointerDevice *device = nullptr)
        : QQuickPointerEvent(parent, device), m_gesturePoint(new QQuickEventPoint(this)) { }

    QQuickPointerEvent *reset(QEvent *) override;
    void localize(QQuickItem *target) override;
    bool isPressEvent() const override;
    bool isUpdateEvent() const override;
    bool isReleaseEvent() const override;
    QQuickPointerNativeGestureEvent *asPointerNativeGestureEvent() override { return this; }
    const QQuickPointerNativeGestureEvent *asPointerNativeGestureEvent() const override { return this; }
    int pointCount() const override { return 1; }
    QQuickEventPoint *point(int i) const override;
    QQuickEventPoint *pointById(int pointId) const override;
    bool allPointsAccepted() const override;
    bool allUpdatedPointsAccepted() const override;
    bool allPointsGrabbed() const override;
    QVector<QObject *> exclusiveGrabbers() const override;
    void clearGrabbers() const override;
    bool hasExclusiveGrabber(const QQuickPointerHandler *handler) const override;
    Qt::NativeGestureType type() const;
    qreal value() const;

private:
    QQuickEventPoint *m_gesturePoint;

    Q_DISABLE_COPY(QQuickPointerNativeGestureEvent)
};
#endif // QT_CONFIG(gestures)


// ### Qt 6: move this to qtbase, replace QTouchDevice and the enums in QTabletEvent
class Q_QUICK_PRIVATE_EXPORT QQuickPointerDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DeviceType type READ type CONSTANT)
    Q_PROPERTY(PointerType pointerType READ pointerType CONSTANT)
    Q_PROPERTY(Capabilities capabilities READ capabilities CONSTANT)
    Q_PROPERTY(int maximumTouchPoints READ maximumTouchPoints CONSTANT)
    Q_PROPERTY(int buttonCount READ buttonCount CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QPointingDeviceUniqueId uniqueId READ uniqueId CONSTANT)

public:
    enum DeviceType : qint16 {
        UnknownDevice = 0x0000,
        Mouse = 0x0001,
        TouchScreen = 0x0002,
        TouchPad = 0x0004,
        Puck = 0x0008,
        Stylus = 0x0010,
        Airbrush = 0x0020,
        AllDevices = 0x7FFF
    };
    Q_DECLARE_FLAGS(DeviceTypes, DeviceType)
    Q_ENUM(DeviceType)
    Q_FLAG(DeviceTypes)

    enum PointerType : qint16 {
        GenericPointer = 0x0001,
        Finger = 0x0002,
        Pen = 0x0004,
        Eraser = 0x0008,
        Cursor = 0x0010,
        AllPointerTypes = 0x7FFF
    };
    Q_DECLARE_FLAGS(PointerTypes, PointerType)
    Q_ENUM(PointerType)
    Q_FLAG(PointerTypes)

    enum CapabilityFlag : qint16 {
        Position    = QTouchDevice::Position,
        Area        = QTouchDevice::Area,
        Pressure    = QTouchDevice::Pressure,
        Velocity    = QTouchDevice::Velocity,
        MouseEmulation = QTouchDevice::MouseEmulation,
        // some bits reserved in case we need more of QTouchDevice::Capabilities
        Scroll      = 0x0100, // mouse has a wheel, or there is OS-level scroll gesture recognition (dubious?)
        Hover       = 0x0200,
        Rotation    = 0x0400,
        XTilt       = 0x0800,
        YTilt       = 0x1000
    };
    Q_DECLARE_FLAGS(Capabilities, CapabilityFlag)
    Q_ENUM(CapabilityFlag)
    Q_FLAG(Capabilities)

    DeviceType type() const { return m_deviceType; }
    PointerType pointerType() const { return m_pointerType; }
    Capabilities capabilities() const { return static_cast<Capabilities>(m_capabilities); }
    bool hasCapability(CapabilityFlag cap) { return m_capabilities & cap; }
    int maximumTouchPoints() const { return m_maximumTouchPoints; }
    int buttonCount() const { return m_buttonCount; }
    QString name() const { return m_name; }
    QPointingDeviceUniqueId uniqueId() const { return m_uniqueId; }

    static QQuickPointerDevice *touchDevice(const QTouchDevice *d);
    static QList<QQuickPointerDevice *> touchDevices();
    static QQuickPointerDevice *genericMouseDevice();
    static QQuickPointerDevice *tabletDevice(qint64);

    QVector<QQuickPointerHandler *> &eventDeliveryTargets() { return m_eventDeliveryTargets; }

private:
    QQuickPointerDevice(DeviceType devType, PointerType pType, Capabilities caps, int maxPoints, int buttonCount, const QString &name, qint64 uniqueId = 0)
      : m_deviceType(devType), m_pointerType(pType), m_capabilities(static_cast<qint16>(caps))
      , m_maximumTouchPoints(static_cast<qint8>(maxPoints)), m_buttonCount(static_cast<qint8>(buttonCount)), m_name(name)
      , m_uniqueId(QPointingDeviceUniqueId::fromNumericId(uniqueId))
    {
    }
    ~QQuickPointerDevice() override { }

private:
    // begin 64-bit field
    DeviceType m_deviceType;
    PointerType m_pointerType;
    qint16 m_capabilities;
    qint8 m_maximumTouchPoints;
    qint8 m_buttonCount;
    // end 64-bit field
    QString m_name;
    QPointingDeviceUniqueId m_uniqueId;
    QVector<QQuickPointerHandler *> m_eventDeliveryTargets; // during delivery, handlers which have already seen the event

    Q_DISABLE_COPY(QQuickPointerDevice)
    friend struct ConstructableQQuickPointerDevice;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPointerDevice::DeviceTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPointerDevice::PointerTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPointerDevice::Capabilities)

Q_QUICK_PRIVATE_EXPORT QDebug operator<<(QDebug, const QQuickPointerDevice *);
Q_QUICK_PRIVATE_EXPORT QDebug operator<<(QDebug, const QQuickPointerEvent *);
Q_QUICK_PRIVATE_EXPORT QDebug operator<<(QDebug, const QQuickEventPoint *);
//Q_QUICK_PRIVATE_EXPORT QDebug operator<<(QDebug, const QQuickEventTouchPoint *); TODO maybe

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickKeyEvent)
QML_DECLARE_TYPE(QQuickMouseEvent)
QML_DECLARE_TYPE(QQuickWheelEvent)
QML_DECLARE_TYPE(QQuickCloseEvent)
QML_DECLARE_TYPE(QQuickPointerDevice)
QML_DECLARE_TYPE(QPointingDeviceUniqueId)
QML_DECLARE_TYPE(QQuickPointerEvent)

#endif // QQUICKEVENTS_P_P_H
