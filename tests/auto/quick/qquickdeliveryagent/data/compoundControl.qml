// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Item {
    id: root
    objectName: "root"
    width: 320
    height: 240

    FocusScope {
        id: spinboxFocusScope
        objectName: "spinboxFocusScope"
        width: spinbox.width
        height: spinbox.height
        SpinBox {
            id: spinbox
            objectName: "spinbox"
            editable: true
            contentItem: TextField {
                objectName: "spinboxContentItem"
            }
        }
    }
}