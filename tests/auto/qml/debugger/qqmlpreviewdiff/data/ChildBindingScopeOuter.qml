// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Outer instantiation — provides the instance-level CU for the wrapper.

import QtQuick

Item {
    id: outerRoot
    property alias inner: innerWrapper

    ChildBindingScopeWrapper {
        id: innerWrapper
    }
}
