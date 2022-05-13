// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    // SourceSize is in device-independent pixels. These images
    // should display at the same visual size.
    Column {
        anchors.centerIn: parent.Center
        Row {

            // standard image with no @2x version
            Image {
                sourceSize.width: 75
                sourceSize.height: 75
                source : "heart-lowdpi.png"
            }

            // image with "@2x" version on disk
            Image {
                sourceSize.width: 75
                sourceSize.height: 75
                source : "heart.png"
            }

            // direct use of "@2x" image
            Image {
                sourceSize.width: 75
                sourceSize.height: 75
                source : "heart-highdpi@2x.png"
            }
        }
    }
}
