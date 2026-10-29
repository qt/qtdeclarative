// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml
import Test

GroupedHolder {
    objectName: "value: " + child.value

    child {
        Binding on value { value: 5 }
    }
}
