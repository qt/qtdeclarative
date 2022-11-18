// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.CheckBox {
    id: control
    objectName: "checkbox-override"

    indicator: Item {
        objectName: "checkbox-indicator-override"
    }

    contentItem: Item {
        objectName: "checkbox-contentItem-override"
    }
}
