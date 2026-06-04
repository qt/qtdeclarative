// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mimics ChoosingCoffeeForm.ui.qml: instantiates the form type and aliases it.
// This is the instance-level CU during rebuild — it does NOT set the signal handler.

import QtQuick

Item {
    id: outerRoot
    property alias target: innerTarget

    SubObjectSignalFormOld {
        id: innerTarget
    }
}
