// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.Templates as T
import QtQuick.Effects

Rectangle {
    id: handle
    width: 20
    height: 20
    radius: 10
    color: Application.styleHints.colorScheme === Qt.Light
        ? Qt.darker(palette.base, down ? 1.05 : 1)
        : Qt.lighter("#cdcbc9", down ? 1.05 : 1)

    required property bool down

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        blurMax: 10
        shadowBlur: 0.2
        shadowScale: 0.92
        shadowOpacity: 1
    }
}
