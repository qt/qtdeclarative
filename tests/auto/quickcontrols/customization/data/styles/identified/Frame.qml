// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Frame {
    id: control
    objectName: "frame-identified"

    contentItem: Item {
        id: contentItem
        objectName: "frame-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "frame-background-identified"
    }
}
