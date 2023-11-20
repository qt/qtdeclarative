// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton
import QtQml

QtObject {
    readonly property url back:         Qt.resolvedUrl("images/back.png")
    readonly property url checkmark:    Qt.resolvedUrl("images/checkmark.png")
    readonly property url next:         Qt.resolvedUrl("images/next.png")
    readonly property url qtLogo:       Qt.resolvedUrl("images/qt-logo.png")
    readonly property url sliderHandle: Qt.resolvedUrl("images/slider_handle.png")
    readonly property url tab:          Qt.resolvedUrl("images/tab.png")
}
