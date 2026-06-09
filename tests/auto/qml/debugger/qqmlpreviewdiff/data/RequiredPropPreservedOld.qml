// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Old version: required property, no extra constant.
import QtQml
QtObject {
    required property QtObject dependency
    property int value: dependency ? dependency.answer : -1
}
