// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mimics ChoosingCoffee.qml: extends the outer form and attaches a signal
// handler to a sub-object of a child component instance using grouped property
// syntax. This is the pattern "cappuccino.button.onClicked: appFlow.cappuccino()".
// The handler comes from THIS CU, which is NOT the instance-level CU for the
// form being rebuilt — that's SubObjectSignalOuterForm.qml.

import QtQuick

SubObjectSignalOuterForm {
    id: wrapper
    property int callCount: 0

    target.button.onTriggered: wrapper.callCount++
}
