// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.3

Item {
    width: 400; height: 400

    ListView {
        id: view
        anchors.top: header.bottom
        anchors.bottom: footer.top
        width: parent.width

        cacheBuffer: 0
        displayMarginBeginning: 60
        displayMarginEnd: 60

        model: 100
        delegate: Rectangle {
            objectName: "delegate"
            width: view.width
            height: 25
            color: index % 2 ? "steelblue" : "lightsteelblue"
            Text {
                anchors.centerIn: parent
                text: index
            }
        }
    }

    Rectangle {
        id: header
        width: parent.width; height: 60
        color: "#80FF0000"
    }

    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        width: parent.width; height: 60
        color: "#80FF0000"
    }
}
