// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

ImageViewer {
    id: viewer

    currentImage: "http://qt.nokia.com/logo.png"

    Text { text: viewer.currentImage }
}
//![0]
