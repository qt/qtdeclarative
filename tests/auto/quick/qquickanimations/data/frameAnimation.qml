// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Rectangle {
    id: root
    width: 200
    height: 200
    visible: true

    property alias frameAnimation: frameAnimation

    FrameAnimation {
        id: frameAnimation
        onTriggered: {
            // Pause when we reach the frame 3
            if (currentFrame === 3)
                pause();
        }
    }
}
