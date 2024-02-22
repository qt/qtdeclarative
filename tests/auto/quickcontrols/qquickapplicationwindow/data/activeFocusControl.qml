// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias container_column: column
    property alias textInput_column: ti_column
    property alias textEdit_column: te_column
    property alias textField_column: tf_column
    property alias textArea_column: ta_column
    property alias spinBox_column: sp_column
    property alias spinContent_column: sp_column.contentItem

    property alias container_frame: frame
    property alias textInput_frame: ti_frame
    property alias textEdit_frame: te_frame
    property alias textField_frame: tf_frame
    property alias textArea_frame: ta_frame
    property alias spinBox_frame: sp_frame
    property alias spinContent_frame: sp_frame.contentItem

    Column {
        id: column

        TextInput {
            id: ti_column
        }
        TextEdit {
            id: te_column
        }
        TextField {
            id: tf_column
        }
        TextArea {
            id: ta_column
        }
        SpinBox {
            id: sp_column
        }
    }

    Frame {
        id: frame

        Column {
            TextInput {
                id: ti_frame
            }
            TextEdit {
                id: te_frame
            }
            TextField {
                id: tf_frame
            }
            TextArea {
                id: ta_frame
            }
            SpinBox {
                id: sp_frame
            }
        }
    }
}
