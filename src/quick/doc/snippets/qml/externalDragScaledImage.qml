// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 200; height: 200

    Image {
        anchors.centerIn: parent
        source: "images/qt_logo.svg"
        sourceSize.width: 96

        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: {
            "text/plain": "Qt Quick rocks!"
        }
        Drag.imageSource: "images/qt_logo.svg"
        Drag.imageSourceSize: Qt.size(48, 35)
        Drag.active: dragHandler.active

        DragHandler {
            id: dragHandler
        }
    }
}
//![0]
