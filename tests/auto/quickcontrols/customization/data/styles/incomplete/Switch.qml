// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Switch {
    id: control
    objectName: "switch-incomplete"

    indicator: Item {
        objectName: "switch-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "switch-contentItem-incomplete"
    }
}
