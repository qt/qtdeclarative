// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
// Images.qml
import QtQuick

Item {
    component LabeledImage: Column {
        property alias source: image.source
        property alias caption: text.text

        Image {
            id: image
            width: 50
            height: 50
        }
        Text {
            id: text
            font.bold: true
        }
    }

    Row {
        LabeledImage {
            id: before
            source: "before.png"
            caption: "Before"
        }
        LabeledImage {
            id: after
            source: "after.png"
            caption: "After"
        }
    }
    property LabeledImage selectedImage: before
}
//! [document]
