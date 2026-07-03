// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A derived type (DerivedWidget) that adds its own property (localCount, with a
// localCountChanged signal) and its own method (combined()) on top of the
// composite base Widget. Its instances live in this (un-reloaded) CU, so their
// caches must be relinked when Widget's layout changes on reload.

pragma ComponentBehavior: Bound

import QtQuick

Item {
    component DerivedWidget: Widget {
        property int localCount: 3
        function combined() { return localCount + value; }
    }

    DerivedWidget { objectName: "w1" }
    DerivedWidget { objectName: "w2" }
}
