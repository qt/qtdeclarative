// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RadioDelegate {
    id: control
    objectName: "radiodelegate-incomplete"

    indicator: Item {
        objectName: "radiodelegate-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "radiodelegate-contentItem-incomplete"
    }

    background: Item {
        objectName: "radiodelegate-background-incomplete"
    }
}
