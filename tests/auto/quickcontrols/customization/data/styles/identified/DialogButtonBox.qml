// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.DialogButtonBox {
    id: control
    objectName: "dialogbuttonbox-identified"

    contentItem: Item {
        id: contentItem
        objectName: "dialogbuttonbox-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "dialogbuttonbox-background-identified"
    }
}
