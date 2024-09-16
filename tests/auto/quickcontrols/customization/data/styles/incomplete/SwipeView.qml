// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.SwipeView {
    id: control
    objectName: "swipeview-incomplete"

    contentItem: Item {
        objectName: "swipeview-contentItem-incomplete"
    }

    background: Item {
        objectName: "swipeview-background-incomplete"
    }
}
