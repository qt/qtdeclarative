// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    width: 320
    height: 240

    ListView {
        id: listView
        width: 320
        height: 240
        delegate: Rectangle {
            width: ListView.view.width
            height: ListView.view.height / ListView.view.count
            color: "tomato"
        }
    }
}
