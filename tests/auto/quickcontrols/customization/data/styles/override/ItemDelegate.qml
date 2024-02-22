// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.ItemDelegate {
    id: control
    objectName: "itemdelegate-override"

    contentItem: Item {
        objectName: "itemdelegate-contentItem-override"
    }

    background: Item {
        objectName: "itemdelegate-background-override"
    }
}
