// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Tumbler {
    id: control
    objectName: "tumbler-identified"

    contentItem: ListView {
        id: contentItem
        objectName: "tumbler-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "tumbler-background-identified"
    }
}
