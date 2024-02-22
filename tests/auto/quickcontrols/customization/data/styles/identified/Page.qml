// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Page {
    id: control
    objectName: "page-identified"

    contentItem: Item {
        id: contentItem
        objectName: "page-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "page-background-identified"
    }
}
