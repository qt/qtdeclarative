// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: myRect

    property FocusRect focusItem
    property string text

    width: 75; height: 50
    radius: 6
    color: "#646464"
    border.width: 4; border.color: "white"

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            myRect.focusItem.x = myRect.x;
            myRect.focusItem.y = myRect.y;
            myRect.focusItem.text = myRect.text;
        }
    }
}
