// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.SpinBox {
    id: control
    objectName: "spinbox-override"

    up.indicator: Item {
        objectName: "spinbox-up.indicator-override"
    }

    down.indicator: Item {
        objectName: "spinbox-down.indicator-override"
    }

    contentItem: Item {
        objectName: "spinbox-contentItem-override"
    }

    background: Item {
        objectName: "spinbox-background-override"
    }
}
