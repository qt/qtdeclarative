// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.ApplicationWindow {
    id: control
    objectName: "applicationwindow-override"

    background: Item {
        objectName: "applicationwindow-background-override"
    }
}
