// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.8
import QtQuick.Window 2.1

Item {
    property int escapeHandlerActivationCount: 0
    property int shortcutActivationCount: 0
    property alias escapeItem: escapeItem

    Item {
        id: escapeItem
        objectName: "escapeItem"
        focus: true

        // By accepting shortcut override events when the key is Qt.Key_Escape,
        // we can ensure that our Keys.onEscapePressed handler (below) will be called.
        Keys.onShortcutOverride: event.accepted = (event.key === Qt.Key_Escape)

        Keys.onEscapePressed: {
            // Pretend that we just did some really important stuff that was triggered
            // by the escape key (like might occur in a popup that has a keyboard shortcut editor, for example).
            // Now that we're done, we no longer need focus, so we won't accept future shorcut override events.
            focus = false;
            event.accepted = true;
            ++escapeHandlerActivationCount;
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: ++shortcutActivationCount
    }
}
