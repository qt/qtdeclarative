// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.RadioButton {
    id: control
    objectName: "radiobutton-override"

    indicator: Item {
        objectName: "radiobutton-indicator-override"
    }

    contentItem: Item {
        objectName: "radiobutton-contentItem-override"
    }
}
