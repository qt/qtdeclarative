// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2
import QtQuick.Controls 2
ApplicationWindow {
    visible: true
    width: 640
    height: 480

    ListView {
        anchors.fill: parent
        model: 2

        delegate: SwipeDelegate {
            text: "Swipe me left (should not crash)"

            swipe.right: Label {
                text: "Release (should not crash)"
            }

            swipe.onCompleted: {
                swipe.close()
            }
        }
    }
}
