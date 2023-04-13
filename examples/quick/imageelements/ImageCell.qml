// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Item {
    property alias mode: image.fillMode
    property alias caption: captionItem.text

    Image {
        id: image

        width: parent.width
        height: parent.height - captionItem.height
        source: "pics/qt-logo.png"
        clip: true      // only makes a difference if mode is PreserveAspectCrop
    }

    Label {
        id: captionItem

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }
}
