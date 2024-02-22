// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.StackView {
    id: control
    objectName: "stackview-identified"

    contentItem: Item {
        id: contentItem
        objectName: "stackview-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "stackview-background-identified"
    }
}
