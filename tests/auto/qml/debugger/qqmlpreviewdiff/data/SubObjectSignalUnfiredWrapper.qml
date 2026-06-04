// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Exercises the coffee demo pattern where the signal is NEVER fired before
// hot-reload. The handler's NotifierEndpoint may remain in the NotifyList's
// pending 'todo' queue if layout() was never triggered for that signal index.
// Without the layout() fix, the handler is silently lost during rebuild.

import QtQuick

SubObjectSignalUnfiredOuterForm {
    id: wrapper
    property int callCount: 0

    target.button.onTriggered: wrapper.callCount++
}
