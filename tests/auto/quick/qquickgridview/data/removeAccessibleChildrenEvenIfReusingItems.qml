// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Rectangle {
    id: page
    width: 640
    height: 480

    Accessible.role : Accessible.Client

    GridView {
        id: grid

        width: 640
        height: 480
        model: listModel

        cellWidth: 180
        cellHeight: 30

        delegate: Text {
            id: textDelegate
            text: name
            Accessible.role: Accessible.Button
            Accessible.name: name
        }
        reuseItems: true
        Accessible.role : Accessible.Client
    }

    ListModel {
        id: listModel
        ListElement { name: "item11"}
        ListElement { name: "item12"}
        ListElement { name: "item13"}
        ListElement { name: "item14"}
    }

    function replaceItems() {
        listModel.clear()
        listModel.insert(0, {"name" : "item21"})
        listModel.insert(1, {"name" : "item22"})
        listModel.insert(2, {"name" : "item23"})
        listModel.insert(3, {"name" : "item24"})
    }
}
