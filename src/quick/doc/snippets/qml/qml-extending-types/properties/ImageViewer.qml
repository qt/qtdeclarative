// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// ImageViewer.qml
import QtQuick

Item {
    id: item
    width: 200; height: 200

    property string currentImage: "default-image.png"

    Image { source: item.currentImage }
}
//![0]
