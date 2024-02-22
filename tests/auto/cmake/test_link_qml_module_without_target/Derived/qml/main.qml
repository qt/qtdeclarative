// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import Base
import Derived

RowLayout {
    width: 200
    height: 200

    Red {
        id: red
        width: 100
        height: 100
    }

    Blue {
        width: 100
        height: 100
    }
}
