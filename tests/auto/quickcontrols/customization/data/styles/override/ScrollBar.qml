// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.ScrollBar {
    id: control
    objectName: "scrollbar-override"

    contentItem: Item {
        objectName: "scrollbar-contentItem-override"
    }

    background: Item {
        objectName: "scrollbar-background-override"
    }
}
