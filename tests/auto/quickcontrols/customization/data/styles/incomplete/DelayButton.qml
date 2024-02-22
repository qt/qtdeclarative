// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.DelayButton {
    id: control
    objectName: "delaybutton-incomplete"

    contentItem: Item {
        objectName: "delaybutton-contentItem-incomplete"
    }

    background: Item {
        objectName: "delaybutton-background-incomplete"
    }
}
