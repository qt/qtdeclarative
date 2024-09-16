// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.SwipeDelegate {
    id: control
    objectName: "swipedelegate-incomplete"

    contentItem: Item {
        objectName: "swipedelegate-contentItem-incomplete"
    }

    background: Item {
        objectName: "swipedelegate-background-incomplete"
    }
}
