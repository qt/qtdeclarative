// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.AbstractButton {
    id: control
    objectName: "abstractbutton-override"

    indicator: Item {
        objectName: "abstractbutton-indicator-override"
    }

    contentItem: Item {
        objectName: "abstractbutton-contentItem-override"
    }

    background: Item {
        objectName: "abstractbutton-background-override"
    }
}
