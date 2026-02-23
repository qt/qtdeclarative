// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Attached property (Keys) with no extra children.
// The Keys attached object sits at a certain index in the CU.
import QtQuick

Item {
    Keys.enabled: true
    property int marker: 1
}
