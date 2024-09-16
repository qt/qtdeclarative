// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    id: root

    signal toggleRequested()
    signal selectRequested()
    signal contextMenuRequested()

    // handler for mouse device
    TapHandler {
        acceptedDevices: PointerDevice.Mouse
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        longPressThreshold: 0
        onTapped: function(event, button) {
            const no_modifier = (point.modifiers === Qt.NoModifier)
            const control_modifier = (point.modifiers === Qt.ControlModifier)
            if (!no_modifier && !control_modifier) {
                 // reject event with other modifiers
                event.accepted = false
                return
            }
            switch (button) {
            case Qt.LeftButton:
                if (control_modifier)
                    root.toggleRequested()
                else if (no_modifier)
                    root.selectRequested()
                break
            case Qt.RightButton:
                if (!no_modifier) {
                    // reject event if there is a modifier
                    event.accepted = false
                    return
                }
                root.contextMenuRequested()
                break
            }
        }
    }

    // handler for touch device
    TapHandler {
        acceptedDevices: PointerDevice.TouchScreen
        acceptedModifiers: Qt.NoModifier
        dragThreshold: 0
        onTapped: (event, button) => root.toggleRequested()
        onLongPressed: () => root.contextMenuRequested()
    }
}
