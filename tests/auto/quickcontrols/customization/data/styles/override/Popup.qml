// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Popup {
    id: control
    objectName: "popup-override"

    contentItem: Item {
        objectName: "popup-contentItem-override"
    }

    background: Item {
        objectName: "popup-background-override"
    }
}
