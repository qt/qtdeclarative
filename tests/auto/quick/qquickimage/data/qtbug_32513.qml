// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    width: 200; height: 200

    Image {
        cache: false

        NumberAnimation on opacity {
            loops: Animation.Infinite
            from: 1; to: 0
        }

        SequentialAnimation on source {
            loops: Animation.Infinite
            PropertyAction { value: "green.png" }
            PauseAnimation { duration: 100 }
            PropertyAction {  value: "pattern.png" }
            PauseAnimation { duration: 100 }
        }
    }
}
