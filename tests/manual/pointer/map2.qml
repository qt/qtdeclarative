// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    width: 640
    height: 480

    Rectangle {
        id: map
        color: "aqua"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: image.implicitWidth
        height: image.implicitHeight
        property point center : Qt.point(x + map.width/2, y + map.height/2)

        function setCenter(xx, yy) {
            map.x = xx - map.width/2
            map.y = yy - map.height/2
        }


        Image {
            id: image
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            source: "resources/map.svgz"
        }
    }

    PinchHandler {
        id: pinch
        target: map
        minimumScale: 0.1
        maximumScale: 10
    }

    DragHandler {
        property point startDrag
        target: null
        onActiveChanged: {
            if (active)
                startDrag = map.center
        }

        onActiveTranslationChanged: {
            map.setCenter(startDrag.x + activeTranslation.x, startDrag.y + activeTranslation.y)
        }
    }
}
