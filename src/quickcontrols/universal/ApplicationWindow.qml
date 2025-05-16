// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Controls.Universal
import QtQuick.Controls.Universal.impl

T.ApplicationWindow {
    id: window

    color: Universal.background

    FocusRectangle {
        parent: window.activeFocusControl
        width: parent ? parent.width : 0
        height: parent ? parent.height : 0
        visible: parent && !!parent.useSystemFocusVisuals && !!parent.visualFocus
    }
}
