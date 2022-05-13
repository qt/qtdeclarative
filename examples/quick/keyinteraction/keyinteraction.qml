// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Loader {//Just loader, since there's only one.
    source: "focus.qml"
    focus: true
    // Exception to the standard sizing, because this is primarily a desktop
    // example and it benefits from being able to see everything at once.
}
