// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    // These images should all have the same visual size:
    Row {
        anchors.centerIn: parent
        // 1X only
        Image {
            source : "heart-lowdpi.png"
        }
        // 1X and 2x, switches on screen change.
        Image {
            source : "heart.png"
        }
        // 2x only
        Image {
            source : "heart-highdpi@2x.png"
        }
    }
}
