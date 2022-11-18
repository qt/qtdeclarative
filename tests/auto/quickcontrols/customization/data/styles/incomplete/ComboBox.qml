// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ComboBox {
    id: control
    objectName: "combobox-incomplete"

    indicator: Item {
        objectName: "combobox-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "combobox-contentItem-incomplete"
    }

    background: Item {
        objectName: "combobox-background-incomplete"
    }

    popup: T.Popup {
        objectName: "combobox-popup-incomplete"
    }
}
