// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict

import QtQuick

Window {
    // If static properties of the Math global object are not directly
    // supported, a warning should be issued in turn failing the build
    // due to `pragma Strict`.
    width: 200 * Math.PI
}
