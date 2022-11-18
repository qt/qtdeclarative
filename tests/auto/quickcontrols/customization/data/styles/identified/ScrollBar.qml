// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ScrollBar {
    id: control
    objectName: "scrollbar-identified"

    contentItem: Item {
        id: contentItem
        objectName: "scrollbar-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "scrollbar-background-identified"
    }
}
