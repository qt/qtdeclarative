// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
// LabeledImageBox.qml
import QtQuick

Rectangle {
    property alias caption: image.caption
    property alias source: image.source
    border.width: 2
    border.color: "black"
    Images.LabeledImage {
        id: image
    }
}
//! [document]
