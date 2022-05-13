// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Flickable {
    objectName: "flickable"
    width: 400; height: 400

    contentHeight: height
    contentWidth: width * 3
    contentX: 400

    Row {
        Rectangle { width: 400; height: 400; color: "green" }
        Rectangle {
            width: 400; height: 400; color: "blue"
            clip: true

            PathView {
                id: pathView
                objectName: "pathView"
                width: parent.width
                height: 200
                anchors.verticalCenter: parent.verticalCenter

                dragMargin: 400
                pathItemCount: 6

                model: 10
                path: Path {
                    startX: -pathView.width / 2
                    startY: pathView.height / 2
                    PathLine { x: pathView.width + pathView.width / 2; y: pathView.height / 2 }
                }

                delegate: Rectangle {
                    width: 100; height: 200
                    color: "purple"
                    MouseArea {
                        anchors.fill: parent
                    }
                }
            }
        }
        Rectangle { width: 400; height: 400; color: "yellow" }
    }
}
