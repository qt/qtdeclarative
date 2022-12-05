// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.RoundButton {
    id: control
    objectName: "roundbutton-override"

    contentItem: Item {
        objectName: "roundbutton-contentItem-override"
    }

    background: Item {
        objectName: "roundbutton-background-override"
    }
}
