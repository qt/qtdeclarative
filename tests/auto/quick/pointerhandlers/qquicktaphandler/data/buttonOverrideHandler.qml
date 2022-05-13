// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    width: 320
    height: 240
    Button {
        id: button
        objectName: "Overridden"
        label: "Overridden"
        x: 10; y: 10; width: parent.width - 20; height: 40
        TapHandler {
            gesturePolicy: TapHandler.ReleaseWithinBounds
            objectName: "override"
            onTapped: button.tapped()
        }
    }
}
