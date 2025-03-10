// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick.Controls.macOS
import QtQuick.Controls.macOS.impl as MacOSImpl

Menu {
    id: menu
    popupType: Popup.Window

    required property var editor

    MacOSImpl.UndoAction {
        editor: menu.editor
    }
    MacOSImpl.RedoAction {
        editor: menu.editor
    }

    MenuSeparator {}

    MacOSImpl.CutAction {
        editor: menu.editor
    }
    MacOSImpl.CopyAction {
        editor: menu.editor
    }
    MacOSImpl.PasteAction {
        editor: menu.editor
    }
    MacOSImpl.DeleteAction {
        editor: menu.editor
    }

    MenuSeparator {}

    MacOSImpl.SelectAllAction {
        editor: menu.editor
    }
}
