// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
