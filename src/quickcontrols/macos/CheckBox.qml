// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultCheckBox {
    id: control
    readonly property Item __focusFrameTarget: indicator
    readonly property Item __focusFrameStyleItem: indicator

    font.pixelSize: indicator.styleFont(control).pixelSize
}
