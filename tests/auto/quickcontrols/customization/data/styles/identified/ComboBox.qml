// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T
import QtQuick.Templates as T

T.ComboBox {
    id: control
    objectName: "combobox-identified"

    indicator: Item {
        id: indicator
        objectName: "combobox-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "combobox-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "combobox-background-identified"
    }

    popup: T.Popup {
        id: popup
        objectName: "combobox-popup-identified"
    }
}
