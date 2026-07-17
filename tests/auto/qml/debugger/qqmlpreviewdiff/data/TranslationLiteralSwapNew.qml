// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    property string greeting: "world"         // literal binding (was translation)
    property string subject: qsTr("hello")    // translation binding (was literal)
}
