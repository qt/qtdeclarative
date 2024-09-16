// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.GroupBox {
    id: control
    objectName: "groupbox-identified"

    label: Text {
        id: label
        objectName: "groupbox-label-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "groupbox-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "groupbox-background-identified"
    }
}
