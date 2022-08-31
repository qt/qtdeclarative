// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Rectangle {
    id: container

    function show(text) {
        dialogText.text = text;
        container.opacity = 1;
    }

    function hide() {
        container.opacity = 0;
    }

    width: dialogText.width + 20
    height: dialogText.height + 20
    opacity: 0

    Text {
        id: dialogText
        anchors.centerIn: parent
        text: ""
    }

    MouseArea {
        anchors.fill: parent
        onClicked: hide();
    }
}
//![0]
