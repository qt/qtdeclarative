// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    enum Status {
        Unknown,
        Loading,
        Ready,
        Error
    }
    property int value: 42
}
