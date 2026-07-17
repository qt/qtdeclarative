// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    property real factor: 7
    x: factor * 2   // script binding → 14
    y: 42           // literal binding
}
