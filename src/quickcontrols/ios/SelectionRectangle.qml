// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS
import QtQuick.Controls.iOS.impl

T.SelectionRectangle {
    id: control

    topLeftHandle: handle
    bottomRightHandle: handle

    Component {
        id: handle
        Image {
            id: image
            source: IOS.url + "selectionrectangle-handle"
            visible: SelectionRectangle.control.active
            ImageSelector on source {
                states: [
                    {"light": Application.styleHints.colorScheme === Qt.Light},
                    {"dark": Application.styleHints.colorScheme === Qt.Dark}
                ]
            }
        }
    }
}
