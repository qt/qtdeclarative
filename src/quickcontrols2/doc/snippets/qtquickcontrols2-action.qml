// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    //! [action]
    Action {
        id: copyAction
        text: qsTr("&Copy")
        icon.name: "edit-copy"
        shortcut: StandardKey.Copy
        onTriggered: window.activeFocusItem.copy()
    }
    //! [action]

    //! [toolbutton]
    ToolButton {
        id: toolButton
        action: copyAction
    }
    //! [toolbutton]

    //! [menuitem]
    MenuItem {
        id: menuItem
        action: copyAction
        text: qsTr("&Copy selected Text")
    }
    //! [menuitem]
}
