// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import "../simple" as Simple

Simple.ComboBox {
    id: control
    objectName: "combobox-override"

    indicator: Item {
        objectName: "combobox-indicator-override"
    }

    contentItem: Item {
        objectName: "combobox-contentItem-override"
    }

    background: Item {
        objectName: "combobox-background-override"
    }

    popup: T.Popup {
        objectName: "combobox-popup-override"
    }
}
