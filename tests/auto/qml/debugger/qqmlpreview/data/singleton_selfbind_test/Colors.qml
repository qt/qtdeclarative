// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Singleton
import QtQml

QtObject {
    property int baseSize: 100
    property int derivedSize: Colors.baseSize * 2
    property string currentColor: "steelblue"
}
