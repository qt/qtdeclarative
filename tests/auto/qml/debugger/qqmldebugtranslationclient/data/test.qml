// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0


Item {
    id: root
    width: 360
    height: 360
    property var widthFactor: 7
    // used in the multilanguage translator
    property var screenId: "yi"

    Column {
        Text {
            text: qsTr("hello")
            width: root.width / widthFactor
            elide: Text.ElideRight
        }
        Text {
            text: qsTr("short")
            width: root.width / widthFactor
            elide: Text.ElideRight
        }
        Text {
            text: "long not translated text"
            width: root.width / widthFactor
            elide: Text.ElideRight
        }
    }
    // this is necessary to have the test working for different font sizes and dpi settings
    Text {
        id: originHelloTextToGetTheNecessaryWidth
        text: "short"
        opacity: 0
        anchors.bottom: root.bottom
        onWidthChanged: root.width = originHelloTextToGetTheNecessaryWidth.width * widthFactor
    }
}
