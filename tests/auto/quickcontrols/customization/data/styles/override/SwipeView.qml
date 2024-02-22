// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.SwipeView {
    id: control
    objectName: "swipeview-override"

    contentItem: Item {
        objectName: "swipeview-contentItem-override"
    }

    background: Item {
        objectName: "swipeview-background-override"
    }
}
