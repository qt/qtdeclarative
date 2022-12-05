// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.ToolBar {
    id: control
    objectName: "toolbar-override"

    contentItem: Item {
        objectName: "toolbar-contentItem-override"
    }

    background: Item {
        objectName: "toolbar-background-override"
    }
}
