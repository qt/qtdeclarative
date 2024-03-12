// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ComboBox {
    objectName: "comboBox"
    textRole: "label"
    model: ListModel {
        ListElement {
            label: qsTr("Hello")
        }
        ListElement {
            label: qsTr("ListView")
        }
    }
}
