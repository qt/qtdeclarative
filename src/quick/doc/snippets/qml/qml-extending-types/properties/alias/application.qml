// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// application.qml
import QtQuick

ImageViewer {
    id: viewer

    currentImage.source: "http://qt.nokia.com/logo.png"
    currentImage.width: width
    currentImage.height: height
    currentImage.fillMode: Image.Tile

    Text { text: currentImage.source }
}
//![0]
