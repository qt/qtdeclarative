// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Pane {
    id: control
    objectName: "pane-identified"

    contentItem: Item {
        id: contentItem
        objectName: "pane-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "pane-background-identified"
    }
}
