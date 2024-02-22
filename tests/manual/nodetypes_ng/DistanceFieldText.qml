// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    Text {
        id: text1
        renderType: Text.QtRendering
        anchors.top: parent.top
        text: "árvíztűrő tükörfúrógép\nÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP"
    }
    Text {
        renderType: Text.QtRendering
        anchors.bottom: parent.bottom
        text: "the quick brown fox jumps over the lazy dog\nTHE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"
        color: "red"
    }
    Text {
        renderType: Text.QtRendering
        anchors.centerIn: parent
        text: "rotate rotate rotate"
        font.bold: true
        font.pointSize: 20
        color: "green"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Row {
        anchors.top: text1.bottom
        anchors.margins: 10
        Text { renderType: Text.QtRendering; font.pointSize: 24; text: "Normal" }
        Text { renderType: Text.QtRendering; font.pointSize: 24; text: "Raised"; style: Text.Raised; styleColor: "#AAAAAA" }
        Text { renderType: Text.QtRendering; font.pointSize: 24; text: "Outline"; style: Text.Outline; styleColor: "red" }
        Text { renderType: Text.QtRendering; font.pointSize: 24; text: "Sunken"; style: Text.Sunken; styleColor: "#AAAAAA" }
    }
}
