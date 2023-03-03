// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

Item {
    height: 480
    width: 320
    LauncherList {
        anchors.fill: parent
        Component.onCompleted: {
            addExample(qsTr("Hello"), qsTr("An Animated Hello World"), Qt.resolvedUrl("fonts/hello.qml"));
            addExample(qsTr("Fonts"), qsTr("Using various fonts with a Text element"), Qt.resolvedUrl("fonts/fonts.qml"));
            addExample(qsTr("Available Fonts"), qsTr("A list of your available fonts"),  Qt.resolvedUrl("fonts/availableFonts.qml"));
            addExample(qsTr("Banner"), qsTr("Large, scrolling text"), Qt.resolvedUrl("fonts/banner.qml"));
            addExample(qsTr("Img tag"), qsTr("Embedding images into text"), Qt.resolvedUrl("imgtag/imgtag.qml"));
            addExample(qsTr("Text Layout"), qsTr("Flowing text around items"), Qt.resolvedUrl("styledtext-layout.qml"));
        }
    }
}
