// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Frame {
    id: control
    objectName: "frame-override"

    contentItem: Item {
        objectName: "frame-contentItem-override"
    }

    background: Item {
        objectName: "frame-background-override"
    }
}
