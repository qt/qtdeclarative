// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic

Item {
    property var c: Control {
        id: inner

        property var myBinding: Binding {
            inner.background: Item {}

            inner {
                background: Item {}
            }
        }
    }
}