// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
