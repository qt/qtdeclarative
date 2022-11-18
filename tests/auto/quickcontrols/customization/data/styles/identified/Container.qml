// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
