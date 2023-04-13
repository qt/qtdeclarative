// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400
    visible: true
    title: qsTr("Active focus debugging example")

    onActiveFocusItemChanged: print("activeFocusItem: " + activeFocusItem)

    Row {
        TextField {
            objectName: "textField1"
        }
        TextField {
            objectName: "textField2"
        }
    }
}
//! [file]
