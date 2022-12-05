// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Button {
    id: control
    objectName: "button-override"

    contentItem: Item {
        objectName: "button-contentItem-override"
    }

    background: Item {
        objectName: "button-background-override"
    }
}
