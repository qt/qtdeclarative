// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 400
    height: 700

    // the images should have the same display size and appearance on each row.
    Column {
        anchors.centerIn: parent.Center
        Row {
            Image { mirror: true; source : "heart-lowdpi.png" }
            Image { mirror: true; source : "heart.png" }
            Image { mirror: true; source : "heart-highdpi@2x.png" }
        }
    }
}
