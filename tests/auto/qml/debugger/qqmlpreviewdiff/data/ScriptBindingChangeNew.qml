// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml

QtObject {
    objectName: "original"
    property int aux: 1
    property int value: aux * 2   // constant 5 → script binding: forces Reattach
}
