// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickforeignutils_p.h"

QT_BEGIN_NAMESPACE

/*!
   \qmltype eventPoint
   \instantiates QEventPoint
   \inqmlmodule QtQuick
   \brief Qml equivalent for \l QEventPoint.
   \sa handlerPoint

   eventPoint is the Qml value type representation
   of \l QEventPoint.
   It has the same properties as \l QEventPoint.

   The following properties are available:

   \list
   \li \l bool \c eventPoint.accepted: see also \l QEventPoint::accepted
   \li \l PointerDevice \c eventPoint.device: see also \l QEventPoint::device
   \li \l size \c eventPoint.ellipseDiameters: see also \l QEventPoint::ellipseDiameters
   \li \l point \c eventPoint.globalGrabPosition: see also \l QEventPoint::globalGrabPosition
   \li \l point \c eventPoint.globalLastPosition: see also \l QEventPoint::globalLastPosition
   \li \l point \c eventPoint.globalPosition: see also \l QEventPoint::globalPosition
   \li \l point \c eventPoint.globalPressPosition: see also \l QEventPoint::globalPressPosition
   \li \l bool \c eventPoint.grabPosition: see also \l QEventPoint::grabPosition
   \li \l int \c eventPoint.id: see also \l QEventPoint::id
   \li \l point \c eventPoint.lastPosition: see also \l QEventPoint::lastPosition
   \li \l ulong \c eventPoint.lastTimestamp: see also \l QEventPoint::lastTimestamp
   \li \l point \c eventPoint.position: see also \l QEventPoint::position
   \li \l point \c eventPoint.pressPosition: see also \l QEventPoint::pressPosition
   \li \l int \c eventPoint.pressTimestamp: see also \l QEventPoint::pressTimestamp
   \li \l real \c eventPoint.pressure: see also \l QEventPoint::pressure
   \li \l real \c eventPoint.rotation: see also \l QEventPoint::rotation
   \li \l point \c eventPoint.sceneGrabPosition: see also \l QEventPoint::sceneGrabPosition
   \li \l point \c eventPoint.sceneLastPosition: see also \l QEventPoint::sceneLastPosition
   \li \l point \c eventPoint.scenePosition: see also \l QEventPoint::scenePosition
   \li \l ulong \c eventPoint.pressTimestamp: see also \l QEventPoint::pressTimestamp
   \li \l point \c eventPoint.scenePressPosition: see also \l QEventPoint::scenePressPosition
   \li \l enumeration \c eventPoint.state: see also \l QEventPoint::state
   \li \l real \c eventPoint.timeHeld: see also \l QEventPoint::timeHeld
   \li \l ulong \c eventPoint.timestamp: see also \l QEventPoint::timestamp
   \li \l pointingDeviceUniqueId \c eventPoint.uniqueId: see also \l QEventPoint::uniqueId
   \li \l vector2d \c eventPoint.velocity: see also \l QEventPoint::velocity
   \endlist


    State supports the following values:

    \table
    \row
        \li \c Qt.TouchPointUnknownState
        \li Unknown state.
    \row
        \li \c Qt.TouchPointStationary
        \li The event point did not move.
    \row
        \li \c Qt.TouchPointPressed
        \li The touch point or button is pressed.
    \row
        \li \c Qt.TouchPointMoved
        \li The event point was updated.
    \row
        \li \c Qt.TouchPointReleased
        \li The touch point or button was released.
    \endtable

    The States type is a typedef for \l {QFlags} {QFlags<State>}. It stores an OR combination of
    State values. See also \l QEventPoint::States

 */

QT_END_NAMESPACE

#include "moc_qquickforeignutils_p.cpp"
