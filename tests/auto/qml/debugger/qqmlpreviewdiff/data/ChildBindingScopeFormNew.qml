// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version with a trivial change to trigger rebuild.

import QtQuick

Item {
    id: formRoot
    property alias widget: w
    property string label: "hello!"

    ChildBindingScopeWidget {
        id: w
        active: true
    }
}
