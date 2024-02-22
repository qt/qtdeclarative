// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ItemDelegate {
    id: control
    objectName: "itemdelegate-incomplete"

    contentItem: Item {
        objectName: "itemdelegate-contentItem-incomplete"
    }

    background: Item {
        objectName: "itemdelegate-background-incomplete"
    }
}
