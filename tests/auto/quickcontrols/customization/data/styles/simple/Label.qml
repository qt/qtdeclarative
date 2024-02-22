// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Label {
    id: control
    objectName: "label-simple"

    background: Rectangle {
        objectName: "label-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
