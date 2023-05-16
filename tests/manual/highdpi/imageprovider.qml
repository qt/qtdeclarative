// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import "HighDPIImageProvider"

Rectangle {
    width: 400
    height: 400

    Row {
        // should render a sharp image on high-dpi displays. sourceSize is in device-independent
        // pixels; the image provider will be asked to create a 400x400 device pixel image.
        Image {
            sourceSize.width: 200
            sourceSize.height: 200
            source: "image://colors/green"
        }

        // should NOT render a sharp image on retina displays. sourceSize is omitted and
        // the image provider default size will be used (in this case 50x50).
        Image {
            width : 200
            height : 200
            source : "image://colors/red"
        }

    }
}
