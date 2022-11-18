// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control
    objectName: "checkbox-identified"

    indicator: Item {
        id: indicator
        objectName: "checkbox-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "checkbox-contentItem-identified"
    }
}
