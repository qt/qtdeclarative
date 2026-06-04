// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Instantiates SubObjectBindingFormOld and exposes it via alias.
// This is the instance-level CU — it does NOT set the binding on button.

import QtQuick

Item {
    id: outerRoot
    property alias target: innerTarget

    SubObjectBindingFormOld {
        id: innerTarget
    }
}
