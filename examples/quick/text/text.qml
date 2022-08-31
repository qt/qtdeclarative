// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Hello", "An Animated Hello World", Qt.resolvedUrl("fonts/hello.qml"));
            addExample("Fonts", "Using various fonts with a Text element", Qt.resolvedUrl("fonts/fonts.qml"));
            addExample("Available Fonts", "A list of your available fonts",  Qt.resolvedUrl("fonts/availableFonts.qml"));
            addExample("Banner", "Large, scrolling text", Qt.resolvedUrl("fonts/banner.qml"));
            addExample("Img tag", "Embedding images into text", Qt.resolvedUrl("imgtag/imgtag.qml"));
            addExample("Text Layout", "Flowing text around items", Qt.resolvedUrl("styledtext-layout.qml"));
        }
    }
}
