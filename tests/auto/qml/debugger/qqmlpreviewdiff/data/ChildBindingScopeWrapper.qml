// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Derived type using grouped property syntax on the widget's timer.
// This creates a compiled GroupProperty sub-object for "widget.button",
// causing stashExternalState to visit the timer with unit=this CU.

import QtQml
import QtQuick

ChildBindingScopeFormOld {
    id: wrapper
    property int triggerCount: 0

    widget.button.onTriggered: triggerCount++
}
