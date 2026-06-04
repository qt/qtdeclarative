// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Wrapper that attaches a signal handler to a Timer accessed via the list
// property. The handler's CU is this file — not the form's CU and not the
// outer form's CU. After rebuilding the inner form, the handler must survive.

import QtQuick

ListChildOuterForm {
    id: wrapper
    property int callCount: 0

    Component.onCompleted: {
        target.timers[1].triggered.connect(function() { wrapper.callCount++ })
    }
}
