// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Rectangle {
    width: 100
    height: 100

    //![0]
    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds // exclusive grab on press
        onGrabChanged:
            (transition, eventPoint) => {
                switch (transition) {
                    case PointerDevice.GrabExclusive:
                        console.log("took exclusive grab of point", eventPoint.id,
                                    "on", eventPoint.device.name)
                        break
                    case PointerDevice.UngrabExclusive:
                        console.log("gave up exclusive grab of point", eventPoint.id,
                                    "on", eventPoint.device.name)
                        break
                    case PointerDevice.CancelGrabExclusive:
                        console.log("exclusive grab of point", eventPoint.id,
                                    "on", eventPoint.device.name, "has been cancelled")
                        break
                }

                switch (eventPoint.state) {
                    case EventPoint.Pressed:
                        console.log("on press @", eventPoint.position);
                        break
                    case EventPoint.Updated:
                        console.log("on update @", eventPoint.position);
                        break
                    case EventPoint.Released:
                        console.log("on release @", eventPoint.position);
                        break
                    default:
                        console.log(eventPoint.position, "state", eventPoint.state)
                        break
                }
            }
    }
    //![0]
}
