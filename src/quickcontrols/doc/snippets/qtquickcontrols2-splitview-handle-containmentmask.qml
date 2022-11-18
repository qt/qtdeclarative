// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: 200
    height: 100

    //! [1]
    SplitView {
        id: splitView
        anchors.fill: parent

        handle: Rectangle {
            id: handleDelegate
            implicitWidth: 4
            implicitHeight: 4
            color: SplitHandle.pressed ? "#81e889"
                : (SplitHandle.hovered ? Qt.lighter("#c2f4c6", 1.1) : "#c2f4c6")

            containmentMask: Item {
                x: (handleDelegate.width - width) / 2
                width: 64
                height: splitView.height
            }
        }

        Rectangle {
            implicitWidth: 150
            color: "#444"
        }
        Rectangle {
            implicitWidth: 50
            color: "#666"
        }
    }
    //! [1]
}
