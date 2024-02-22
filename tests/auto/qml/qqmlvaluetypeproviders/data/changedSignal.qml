// Copyright (C) 2016 basysKom GmbH, opensource@basyskom.com.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    property bool success: false
    property bool complete: false

    property vector2d v2: Qt.vector2d(-2, 0)
    property vector3d v3: Qt.vector3d(-2, 0, 0)
    property vector4d v4: Qt.vector4d(-2, 0, 0, 0)

    property int v2ChangedSignalCount;
    property int v3ChangedSignalCount;
    property int v4ChangedSignalCount;

    onV2Changed: v2ChangedSignalCount++
    onV3Changed: v3ChangedSignalCount++
    onV4Changed: v4ChangedSignalCount++

    Component.onCompleted: {
        complete = false;
        success = true;

        // storing the initial value causes a signal emission
        if (v2ChangedSignalCount !== 1) success = false
        v2 = Qt.vector2d(-2, 0);
        // setting the same value again must not emit a signal
        if (v2ChangedSignalCount !== 1) success = false
        v2.x++
        if (v2ChangedSignalCount !== 2) success = false
        v2.x++  // cycle through 0, 0 which is the default value
        if (v2ChangedSignalCount !== 3) success = false
        v2.x++
        if (v2ChangedSignalCount !== 4) success = false

        // storing the initial value causes a signal emission
        if (v3ChangedSignalCount !== 1) success = false
        v3 = Qt.vector3d(-2, 0, 0);
        // setting the same value again must not emit a signal
        if (v3ChangedSignalCount !== 1) success = false
        v3.x++
        if (v3ChangedSignalCount !== 2) success = false
        v3.x++ // cycle through 0, 0, 0 which is the default value
        if (v3ChangedSignalCount !== 3) success = false
        v3.x++
        if (v3ChangedSignalCount !== 4) success = false

        // storing the initial value causes a signal emission
        if (v4ChangedSignalCount !== 1) success = false
        v4 = Qt.vector4d(-2, 0, 0, 0);
        // setting the same value again must not emit a signal
        if (v4ChangedSignalCount !== 1) success = false
        v4.x++
        if (v4ChangedSignalCount !== 2) success = false
        v4.x++ // cycle through 0, 0, 0 which is the default value
        if (v4ChangedSignalCount !== 3) success = false
        v4.x++
        if (v4ChangedSignalCount !== 4) success = false

        complete = true;
    }
}
