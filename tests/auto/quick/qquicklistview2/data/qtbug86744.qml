// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQml.Models

Item {
    height: 200
    width: 100
    DelegateModel {
        id: dm
        model: 2
        delegate: Item {
            width: 100
            height: 20
            property bool isCurrent: ListView.isCurrentItem
        }
    }
    ListView {
        objectName: "listView"
        model: dm
        currentIndex: 1
        anchors.fill: parent
    }
}
