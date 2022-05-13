// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12

Item {
    width: 640
    height: 450

    property alias loader: loader
    property TableView tableView: loader.item ? loader.item.tableView : null
    property string loaderSource: ""

    Loader {
        id: loader
        anchors.fill: parent
        source: Qt.resolvedUrl(loaderSource)
        asynchronous: true
    }
}
