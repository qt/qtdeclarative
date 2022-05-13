// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
