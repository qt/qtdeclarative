// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control
    objectName: "button-incomplete"

    contentItem: Item {
        objectName: "button-contentItem-incomplete"
    }

    background: Item {
        objectName: "button-background-incomplete"
    }
}
