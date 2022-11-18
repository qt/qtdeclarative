// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Dialog {
    id: control
    objectName: "dialog-identified"

    contentItem: Item {
        id: contentItem
        objectName: "dialog-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "dialog-background-identified"
    }
}
