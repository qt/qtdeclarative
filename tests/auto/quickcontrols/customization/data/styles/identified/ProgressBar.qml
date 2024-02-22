// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ProgressBar {
    id: control
    objectName: "progressbar-identified"

    contentItem: Item {
        id: contentItem
        objectName: "progressbar-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "progressbar-background-identified"
    }
}
