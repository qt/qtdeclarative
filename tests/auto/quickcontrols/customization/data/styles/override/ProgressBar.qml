// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.ProgressBar {
    id: control
    objectName: "progressbar-override"

    contentItem: Item {
        objectName: "progressbar-contentItem-override"
    }

    background: Item {
        objectName: "progressbar-background-override"
    }
}
