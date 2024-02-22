// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Container {
    id: control
    objectName: "container-identified"

    contentItem: Item {
        id: contentItem
        objectName: "container-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "container-background-identified"
    }
}
