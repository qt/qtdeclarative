// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemsmodule_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickevents_p_p.h"
#include "qquickrectangle_p.h"
#include "qquickfocusscope_p.h"
#include "qquicktext_p.h"
#include "qquicktextinput_p.h"
#include "qquicktextedit_p.h"
#include "qquicktextdocument.h"
#include "qquickimage_p.h"
#include "qquickborderimage_p.h"
#include "qquickscalegrid_p_p.h"
#include "qquickmousearea_p.h"
#include "qquickpincharea_p.h"
#include "qquickflickable_p.h"
#include "qquickflickable_p_p.h"
#if QT_CONFIG(quick_listview)
#include "qquicklistview_p.h"
#endif
#if QT_CONFIG(quick_gridview)
#include "qquickgridview_p.h"
#endif
#if QT_CONFIG(quick_pathview)
#include "qquickpathview_p.h"
#endif
#if QT_CONFIG(quick_tableview)
#include "qquicktableview_p.h"
#endif
#if QT_CONFIG(quick_viewtransitions)
#include "qquickitemviewtransition_p.h"
#endif
#if QT_CONFIG(quick_path)
#include <private/qquickpath_p.h>
#include <private/qquickpathinterpolator_p.h>
#endif
#if QT_CONFIG(quick_positioners)
#include "qquickpositioners_p.h"
#endif
#if QT_CONFIG(quick_repeater)
#include "qquickrepeater_p.h"
#endif
#include "qquickloader_p.h"
#if QT_CONFIG(quick_animatedimage)
#include "qquickanimatedimage_p.h"
#endif
#if QT_CONFIG(quick_flipable)
#include "qquickflipable_p.h"
#endif
#include "qquicktranslate_p.h"
#include "qquickstateoperations_p.h"
#include "qquickitemanimation_p.h"
//#include <private/qquickpincharea_p.h>
#if QT_CONFIG(quick_canvas)
#include <QtQuick/private/qquickcanvasitem_p.h>
#include <QtQuick/private/qquickcontext2d_p.h>
#endif
#include "qquickitemgrabresult.h"
#if QT_CONFIG(quick_sprite)
#include "qquicksprite_p.h"
#include "qquickspritesequence_p.h"
#include "qquickanimatedsprite_p.h"
#endif
#include "qquickgraphicsinfo_p.h"
#if QT_CONFIG(quick_shadereffect)
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include "qquickshadereffect_p.h"
#include "qquickshadereffectmesh_p.h"
#endif
#if QT_CONFIG(quick_draganddrop)
#include "qquickdrag_p.h"
#include "qquickdroparea_p.h"
#endif
#include "qquickmultipointtoucharea_p.h"
#include <QtQuick/private/qquickaccessibleattached_p.h>

#include "handlers/qquickdraghandler_p.h"
#include "handlers/qquickhoverhandler_p.h"
#include "handlers/qquickpinchhandler_p.h"
#include "handlers/qquickpointhandler_p.h"
#include "handlers/qquicktaphandler_p.h"
#include "handlers/qquickwheelhandler_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcTransient)
QT_END_NAMESPACE

#include "moc_qquickitemsmodule_p.cpp"

static QQmlPrivate::AutoParentResult qquickitem_autoParent(QObject *obj, QObject *parent)
{
    // When setting a parent (especially during dynamic object creation) in QML,
    // also try to set up the analogous item/window relationship.
    if (QQuickItem *parentItem = qmlobject_cast<QQuickItem *>(parent)) {
        QQuickItem *item = qmlobject_cast<QQuickItem *>(obj);
        if (item) {
            // An Item has another Item
            item->setParentItem(parentItem);
            return QQmlPrivate::Parented;
        } else if (parentItem->window()) {
            QQuickWindow *win = qmlobject_cast<QQuickWindow *>(obj);
            if (win) {
                // A Window inside an Item should be transient for that item's window
                qCDebug(lcTransient) << win << "is transient for" << parentItem->window();
                win->setTransientParent(parentItem->window());
                return QQmlPrivate::Parented;
            }
        } else if (QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(obj)) {
            QQuickItemPrivate::get(parentItem)->addPointerHandler(handler);
            handler->setParent(parent);
            return QQmlPrivate::Parented;
        }
        return QQmlPrivate::IncompatibleObject;
    } else if (QQuickWindow *parentWindow = qmlobject_cast<QQuickWindow *>(parent)) {
        QQuickWindow *win = qmlobject_cast<QQuickWindow *>(obj);
        if (win) {
            // A Window inside a Window should be transient for it
            qCDebug(lcTransient) << win << "is transient for" << parentWindow;
            win->setTransientParent(parentWindow);
            return QQmlPrivate::Parented;
        } else if (QQuickItem *item = qmlobject_cast<QQuickItem *>(obj)) {
            // The parent of an Item inside a Window is actually the implicit content Item
            item->setParentItem(parentWindow->contentItem());
            return QQmlPrivate::Parented;
        } else if (QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(obj)) {
            QQuickItemPrivate::get(parentWindow->contentItem())->addPointerHandler(handler);
            handler->setParent(parentWindow->contentItem());
            return QQmlPrivate::Parented;
        }
        return QQmlPrivate::IncompatibleObject;
    } else if (qmlobject_cast<QQuickItem *>(obj)) {
        return QQmlPrivate::IncompatibleParent;
    }
    return QQmlPrivate::IncompatibleObject;
}

static void qt_quickitems_defineModule()
{
    QQmlPrivate::RegisterAutoParent autoparent = { 0, &qquickitem_autoParent };
    QQmlPrivate::qmlregister(QQmlPrivate::AutoParentRegistration, &autoparent);

    qRegisterMetaType<QQuickAnchorLine>("QQuickAnchorLine");
    qRegisterMetaType<QQuickHandlerPoint>();
}

//static void initResources()
//{
//    Q_INIT_RESOURCE(items);
//}

QT_BEGIN_NAMESPACE

void QQuickItemsModule::defineModule()
{
//    initResources();
    qt_quickitems_defineModule();
}

/*!
    \qmltype PointerEvent
    \instantiates QPointerEvent
    \inqmlmodule QtQuick
    \brief QML equivalent for \l QPointerEvent.

    PointerEvent is the QML name of the QPointerEvent class.
*/

/*!
    \qmltype PointerDevice
    \instantiates QPointingDevice
    \inqmlmodule QtQuick
    \brief QML equivalent for \l QPointingDevice.

    PointerDevice is the QML name of the QPointingDevice class.
    It has the same properties and enums as \l QPointingDevice.
*/

/*!
    \qmlproperty enumeration PointerDevice::deviceType

    This property tells the type of device that generated a PointerEvent.

    Valid values are:

    \value PointerDevice.Unknown        The device cannot be identified.
    \value PointerDevice.Mouse          A mouse.
    \value PointerDevice.TouchScreen    A touchscreen.
    \value PointerDevice.TouchPad       A touchpad or trackpad.
    \value PointerDevice.Stylus         A stylus on a graphics tablet.
    \value PointerDevice.Airbrush       An airbrush on a graphics tablet.
    \value PointerDevice.Puck           A digitizer with crosshairs, on a graphics tablet.

    \sa QInputDevice::DeviceType, PointerDeviceHandler::acceptedDevices
*/

/*!
    \qmlproperty enumeration PointerDevice::pointerType

    This property tells what is interacting with the PointerDevice.

    There is some redundancy between this property and \l deviceType.
    For example, if a touchscreen is used, then \c deviceType is
    \c TouchScreen and \c pointerType is \c Finger. But on a graphics
    tablet, it's often possible for both ends of the stylus to be used,
    and programs need to distinguish them.
    \l PointerDeviceHandler::acceptedDevices and
    \l PointerDeviceHandler::acceptedPointerTypes can be used in combination
    to filter the subset of events that a particular handler should react to.

    Valid values are:

    \value PointerDevice.Unknown        The device cannot be identified.
    \value PointerDevice.Generic        A mouse or a device that emulates a mouse.
    \value PointerDevice.Finger         A finger on a touchscreen.
    \value PointerDevice.Pen            A stylus on a graphics tablet.
    \value PointerDevice.Eraser         An eraser on a graphics tablet.
    \value PointerDevice.Cursor         A digitizer with crosshairs, on a graphics tablet.

    \sa QPointingDevice::PointerType, PointerDeviceHandler::acceptedPointerTypes
*/

/*!
    \qmlproperty int PointerDevice::maximumPoints

    This property tells the maximum number of simultaneous touch points
    (fingers) that can be detected.
*/

/*!
    \qmlproperty int PointerDevice::buttonCount

    This property tells the maximum number of on-device buttons that can be
    detected.
*/

/*!
    \qmltype pointingDeviceUniqueId
    \instantiates QPointingDeviceUniqueId
    \inqmlmodule QtQuick
    \brief QML equivalent for \l QPointingDeviceUniqueId.

    pointingDeviceUniqueId is the QML name of the QPointingDeviceUniqueId class.
*/

/*!
    \qmlproperty qint64 pointingDeviceUniqueId::numericId

    This property gives the numeric ID of the \l PointerDevice, if available;
    otherwise it is \c -1.
*/

/*!
    \qmlproperty pointingDeviceUniqueId PointerDevice::uniqueId

    This property may provide a unique ID for the device, if available. For
    example, a graphics tablet stylus device may have a unique serial number.

    \sa eventPoint, QEventPoint::uniqueId()
*/

/*!
    \qmlsignal PointerDevice::grabChanged(QtObject grabber, enumeration transition, PointerEvent event, eventPoint point)

    This signal is emitted when the \a grabber object gains or loses an
    exclusive or passive grab of \a point during delivery of \a event.
    The \a transition tells what happened, from the perspective of the
    \c grabber object, which may be either an \l Item or an
    \l {Qt Quick Input Handlers}{Input Handler}.

    Valid values for \a transition are:

    \value PointerDevice.GrabExclusive
        The \a grabber has taken primary responsibility for handling the \a point.
    \value PointerDevice.UngrabExclusive
        The \a grabber has given up its previous exclusive grab.
    \value PointerDevice.CancelGrabExclusive
        The exclusive grab of \a grabber has been taken over or cancelled.
    \value PointerDevice.GrabPassive
        The \a grabber has acquired a passive grab, to monitor the \a point.
    \value PointerDevice.UngrabPassive
        The \a grabber has given up its previous passive grab.
    \value PointerDevice.CancelGrabPassive
        The previous passive grab has terminated abnormally.

    \note A grab transition from one object to another results in two signals,
    to notify that one object has lost its grab, and to notify that there is
    another grabber. In other cases, when transitioning to or from a non-grabbing
    state, only one signal is emitted.

    \sa QPointerEvent::setExclusiveGrabber(), QPointerEvent::addPassiveGrabber(),
        QPointerEvent::removePassiveGrabber(), PointerHandler::grabChanged()
*/

QT_END_NAMESPACE
