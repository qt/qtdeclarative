// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property alias bb: inner.y
    property alias aa: inner.x
    Item { id: inner; x: 5; y: 7 }
}
