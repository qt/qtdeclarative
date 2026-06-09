// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Form that instantiates ChildBindingScopeWidget. Editing this file triggers a
// rebuild of its CU, which recreates the child widget and its sub-objects.

import QtQuick

Item {
    id: formRoot
    property alias widget: w
    property string label: "hello"

    ChildBindingScopeWidget {
        id: w
        active: true
    }
}
