// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// External composite type with a script binding (computed from a constant property).
import QtQuick

Item {
    property int value: 10
    property int squared: value * value
}
