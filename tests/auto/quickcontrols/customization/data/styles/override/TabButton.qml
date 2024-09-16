// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.TabButton {
    id: control
    objectName: "tabbutton-override"

    contentItem: Item {
        objectName: "tabbutton-contentItem-override"
    }

    background: Item {
        objectName: "tabbutton-background-override"
    }
}
