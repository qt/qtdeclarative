// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RoundButton {
    id: control
    objectName: "roundbutton-identified"

    contentItem: Item {
        id: contentItem
        objectName: "roundbutton-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "roundbutton-background-identified"
    }
}
