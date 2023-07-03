// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
import QtQuick

Rectangle {
    width: 240; height: 200

//! [1]
    Component {
        id: delegate
        Item {
            width: 80; height: 80
            scale: PathView.iconScale
            opacity: PathView.iconOpacity
            Column {
                Image { anchors.horizontalCenter: nameText.horizontalCenter; width: 64; height: 64; source: icon }
                Text { id: nameText; text: name; font.pointSize: 16 }
            }
        }
    }
//! [1]

//! [2]
    PathView {
        anchors.fill: parent
        model: ContactModel {}
        delegate: delegate
        path: Path {
            startX: 120; startY: 100
            PathAttribute { name: "iconScale"; value: 1.0 }
            PathAttribute { name: "iconOpacity"; value: 1.0 }
            PathQuad { x: 120; y: 25; controlX: 260; controlY: 75 }
            PathAttribute { name: "iconScale"; value: 0.3 }
            PathAttribute { name: "iconOpacity"; value: 0.5 }
            PathQuad { x: 120; y: 100; controlX: -20; controlY: 75 }
        }
    }
//! [2]
}
//! [0]
