// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.DialogButtonBox {
    id: control
    objectName: "dialogbuttonbox-override"

    contentItem: Item {
        objectName: "dialogbuttonbox-contentItem-override"
    }

    background: Item {
        objectName: "dialogbuttonbox-background-override"
    }
}
