// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    implicitWidth: parent.width
    RowLayout {
        Label {
            text: "Export images as:"
        }
        CheckBox {
            text: "png@1x"
            checked: bridge.isImageFormatSelected(text)
            onCheckedChanged: bridge.selectImageFormat(text, checked)
        }
        CheckBox {
            text: "png@2x"
            checked: bridge.isImageFormatSelected(text)
            onCheckedChanged: bridge.selectImageFormat(text, checked)
        }
        CheckBox {
            text: "png@3x"
            checked: bridge.isImageFormatSelected(text)
            onCheckedChanged: bridge.selectImageFormat(text, checked)
        }
        CheckBox {
            text: "png@4x"
            checked: bridge.isImageFormatSelected(text)
            onCheckedChanged: bridge.selectImageFormat(text, checked)
        }
        CheckBox {
            text: "svg"
            checked: bridge.isImageFormatSelected(text)
            onCheckedChanged: bridge.selectImageFormat(text, checked)
        }
    }
}

