// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
import QtQuick

Rectangle {
    width: 240; height: 200

//! [1]
    Component {
        id: delegate
        Column {
            id: wrapper

            required property url icon
            required property string name

            opacity: PathView.isCurrentItem ? 1 : 0.5

            Image {
                anchors.horizontalCenter: nameText.horizontalCenter
                width: 64; height: 64
                source: wrapper.icon
            }
            Text {
                id: nameText
                text: wrapper.name
                font.pointSize: 16
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
            PathQuad { x: 120; y: 25; controlX: 260; controlY: 75 }
            PathQuad { x: 120; y: 100; controlX: -20; controlY: 75 }
        }
    }
//! [2]
}
//! [0]
