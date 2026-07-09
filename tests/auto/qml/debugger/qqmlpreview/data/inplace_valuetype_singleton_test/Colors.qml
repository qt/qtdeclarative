// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Singleton
import QtQml

QtObject {
    id: root
    property QtObject dark: QtObject {}
    property QtObject light: QtObject {}
    property QtObject currentTheme: dark
}
