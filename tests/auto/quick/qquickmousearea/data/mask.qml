// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.11
import Test 1.0

Item {
    id: root
    property int clicked: 0
    property int pressed: 0
    property int released: 0

    width: 200; height: 200

    MouseArea {
        id: mouseArea
        width: 200; height: 200
        onPressed: { root.pressed++ }
        onClicked: { root.clicked++ }
        onReleased: { root.released++ }

        containmentMask: CircleMask {
            radius: mouseArea.width/2
        }
    }
}

