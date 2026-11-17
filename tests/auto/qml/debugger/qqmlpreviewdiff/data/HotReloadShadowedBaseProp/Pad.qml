// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick

Item {
    component DerivedWidget: Widget {
        property int shadowed: 42
    }

    function readShadowed(o) { return o.shadowed }

    DerivedWidget { objectName: "w" }
}
