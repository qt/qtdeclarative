// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Item {
    width: 320
    height: 240
    Button {
        objectName: "DragThreshold"
        label: "DragThreshold"
        x: 10; y: 10; width: 300; height: 40
        gesturePolicy: TapHandler.DragThreshold
    }
    Button {
        objectName: "WithinBounds"
        label: "WithinBounds"
        x: 10; y: 60; width: 300; height: 40
        gesturePolicy: TapHandler.WithinBounds
    }
    Button {
        objectName: "ReleaseWithinBounds"
        label: "ReleaseWithinBounds"
        x: 10; y: 110; width: 300; height: 40
        gesturePolicy: TapHandler.ReleaseWithinBounds
    }
    Button {
        objectName: "DragWithinBounds"
        label: "DragWithinBounds"
        x: 10; y: 160; width: 300; height: 40
        gesturePolicy: TapHandler.DragWithinBounds
    }
}
