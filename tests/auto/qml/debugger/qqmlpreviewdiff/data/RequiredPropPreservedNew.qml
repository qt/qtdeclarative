// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: adds a trivial constant property to trigger a hot-reload diff.
import QtQml
QtObject {
    required property QtObject dependency
    property int value: dependency ? dependency.answer : -1
    property int marker: 99
}
