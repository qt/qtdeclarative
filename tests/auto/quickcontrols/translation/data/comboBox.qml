// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
