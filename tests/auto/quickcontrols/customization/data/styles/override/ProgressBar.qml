// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
