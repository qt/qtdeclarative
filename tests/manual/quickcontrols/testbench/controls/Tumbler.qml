// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

QtObject {
    property var supportedStates: [
        [],
        ["disabled"]
    ]

    property Component component: Tumbler {
        model: 20
        enabled: !is("disabled")

        LayoutMirroring.enabled: is("mirrored")
    }
}
