// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as SignalRemovedOld but with the fired() signal removed entirely.
// This forces the stashed external handler to fail to reattach.

import QtQml

QtObject {
    id: root
    property int tag: 2
}
