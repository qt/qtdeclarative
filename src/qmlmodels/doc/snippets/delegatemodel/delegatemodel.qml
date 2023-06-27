// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import QtQml.Models

Rectangle {
    width: 200; height: 100

    DelegateModel {
        id: visualModel
        model: ListModel {
            ListElement { name: "Apple" }
            ListElement { name: "Orange" }
        }
        delegate: Rectangle {
            height: 25
            width: 100
            Text { text: "Name: " + name}
        }
    }

    ListView {
        anchors.fill: parent
        model: visualModel
    }
}
//![0]
