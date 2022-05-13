// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.GroupBox {
    id: control
    objectName: "groupbox-override"

    label: Text {
        objectName: "groupbox-label-override"
    }

    contentItem: Item {
        objectName: "groupbox-contentItem-override"
    }

    background: Item {
        objectName: "groupbox-background-override"
    }
}
