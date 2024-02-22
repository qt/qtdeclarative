// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["mirrored"]
    ]

    property Component component: BusyIndicator {
        enabled: !is("disabled")
        LayoutMirroring.enabled: is("mirrored")
    }
}
