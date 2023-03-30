// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    property alias color : rectangle.color

//! [shadow]
    BorderImage {
        anchors.fill: rectangle
        anchors {
            leftMargin: -6
            topMargin: -6
            rightMargin: -8
            bottomMargin: -8
        }
        border {
            left: 10
            top: 10
            right: 10
            bottom: 10
        }
        source: "pics/shadow.png"
    }
//! [shadow]

    Rectangle {
        id: rectangle

        anchors.fill: parent
    }
}
