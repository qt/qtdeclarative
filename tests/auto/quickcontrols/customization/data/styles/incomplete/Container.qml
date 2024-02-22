// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Container {
    id: control
    objectName: "container-incomplete"

    contentItem: Item {
        objectName: "container-contentItem-incomplete"
    }

    background: Item {
        objectName: "container-background-incomplete"
    }
}
