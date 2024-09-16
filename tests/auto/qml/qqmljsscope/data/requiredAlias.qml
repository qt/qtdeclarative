// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: self

    property int nonRequired
    property alias sameScopeAlias: self.nonRequired
    required sameScopeAlias

    property alias innerScopeAlias: inner.nonRequiredInner
    required innerScopeAlias

    Item {
        id: inner
        property int nonRequiredInner
    }
}
