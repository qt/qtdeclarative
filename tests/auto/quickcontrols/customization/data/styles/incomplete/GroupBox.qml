// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.GroupBox {
    id: control
    objectName: "groupbox-incomplete"

    label: Text {
        objectName: "groupbox-label-incomplete"
    }

    contentItem: Item {
        objectName: "groupbox-contentItem-incomplete"
    }

    background: Item {
        objectName: "groupbox-background-incomplete"
    }
}
