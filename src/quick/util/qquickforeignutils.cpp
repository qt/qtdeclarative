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
   \li \l bool \c eventPoint.accepted \br
       See also \l {QEventPoint::accepted}.
   \li \l PointerDevice \c eventPoint.device \br
       See also \l {QEventPoint::device}.
   \li \l size \c eventPoint.ellipseDiameters \br
       See also \l {QEventPoint::ellipseDiameters}.
   \li \l point \c eventPoint.globalGrabPosition \br
       See also \l {QEventPoint::globalGrabPosition}.
   \li \l point \c eventPoint.globalLastPosition \br
       See also \l {QEventPoint::globalLastPosition}.
   \li \l point \c eventPoint.globalPosition \br
       See also \l {QEventPoint::globalPosition}.
   \li \l point \c eventPoint.globalPressPosition \br
       See also \l {QEventPoint::globalPressPosition}.
   \li \l bool \c eventPoint.grabPosition \br
       See also \l {QEventPoint::grabPosition}.
   \li \l int \c eventPoint.id \br
       See also \l {QEventPoint::id}.
   \li \l point \c eventPoint.lastPosition \br
       See also \l {QEventPoint::lastPosition}.
   \li \l ulong \c eventPoint.lastTimestamp \br
       See also \l {QEventPoint::lastTimestamp}.
   \li \l point \c eventPoint.position \br
       See also \l {QEventPoint::position}.
   \li \l point \c eventPoint.pressPosition \br
       See also \l {QEventPoint::pressPosition}.
   \li \l int \c eventPoint.pressTimestamp \br
       See also \l {QEventPoint::pressTimestamp}.
   \li \l real \c eventPoint.pressure \br
       See also \l {QEventPoint::pressure}.
   \li \l real \c eventPoint.rotation \br
       See also \l {QEventPoint::rotation}.
   \li \l point \c eventPoint.sceneGrabPosition \br
       See also \l {QEventPoint::sceneGrabPosition}.
   \li \l point \c eventPoint.sceneLastPosition \br
       See also \l {QEventPoint::sceneLastPosition}.
   \li \l point \c eventPoint.scenePosition \br
       See also \l {QEventPoint::scenePosition}.
   \li \l ulong \c eventPoint.pressTimestamp \br
       See also \l {QEventPoint::pressTimestamp}.
   \li \l point \c eventPoint.scenePressPosition \br
       See also \l {QEventPoint::scenePressPosition}.
   \li \l {QML Enumerations}{enumeration} \c eventPoint.state \br
       See also \l {QEventPoint::state}.
   \li \l real \c eventPoint.timeHeld \br
       See also \l {QEventPoint::timeHeld}.
   \li \l ulong \c eventPoint.timestamp \br
       See also \l {QEventPoint::timestamp}.
   \li \l pointingDeviceUniqueId \c eventPoint.uniqueId \br
       See also \l {QEventPoint::uniqueId}.
   \li \l vector2d \c eventPoint.velocity \br
       See also \l {QEventPoint::velocity}.
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
