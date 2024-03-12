// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Pane {
    id: control
    objectName: "pane-override"

    contentItem: Item {
        objectName: "pane-contentItem-override"
    }

    background: Item {
        objectName: "pane-background-override"
    }
}
