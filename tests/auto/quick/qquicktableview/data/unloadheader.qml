// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.qmlmodels

Item {
    width: 640
    height: 480

    property alias tableView: tableView
    property alias loader: verticalHeaderLoader

    Loader {
        id: verticalHeaderLoader
        x: 0
        width: item ? item.contentWidth : 0
        height: parent.height
        sourceComponent: TableView {
            model: 5
            syncView: tableView
            syncDirection: Qt.Vertical
            delegate: Text {
                text: index
            }
        }
    }

    TableView {
        id: tableView
        anchors {
            left: verticalHeaderLoader.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        model: 5
        delegate: Text {
            text: index
        }
    }
}
