// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadOwnMembers/Widget.qml, but with an extra property (and thus
// an extra "extraChanged" signal) added. This grows the base's meta-object,
// shifting the indices of the derived types' own members.

import QtQuick

Item {
    property int value: 10
    property int extra: 5
}
