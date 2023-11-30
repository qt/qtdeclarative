// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Effects
import WearableStyle

Item {
    Rectangle {
        id: listitem
        width: parent.width
        height: parent.height
        radius: 8
        color: UIStyle.listItemBackground
    }

    MultiEffect {
        source: listitem
        anchors.fill: parent
        shadowEnabled: true
        shadowBlur: 0.3
        shadowHorizontalOffset: 2
        shadowVerticalOffset: 2
        opacity: 0.5
    }
}
