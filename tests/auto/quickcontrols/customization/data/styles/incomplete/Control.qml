// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Control {
    id: control
    objectName: "control-incomplete"

    contentItem: Item {
        objectName: "control-contentItem-incomplete"
    }

    background: Item {
        objectName: "control-background-incomplete"
    }
}
