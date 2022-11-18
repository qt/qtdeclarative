// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Popup {
    id: control
    objectName: "popup-incomplete"

    contentItem: Item {
        objectName: "popup-contentItem-incomplete"
    }

    background: Item {
        objectName: "popup-background-incomplete"
    }
}
