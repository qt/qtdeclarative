// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.SpinBox {
    id: control
    objectName: "spinbox-incomplete"

    up.indicator: Item {
        objectName: "spinbox-up.indicator-incomplete"
    }

    down.indicator: Item {
        objectName: "spinbox-down.indicator-incomplete"
    }

    contentItem: Item {
        objectName: "spinbox-contentItem-incomplete"
    }

    background: Item {
        objectName: "spinbox-background-incomplete"
    }
}
