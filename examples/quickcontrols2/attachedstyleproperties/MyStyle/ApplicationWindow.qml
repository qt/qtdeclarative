// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

import MyStyle

T.ApplicationWindow {
    color: MyStyle.windowColor

    Behavior on color {
        ColorAnimation {
            duration: 150
        }
    }
}
