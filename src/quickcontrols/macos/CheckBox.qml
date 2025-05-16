// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultCheckBox {
    id: control
    readonly property Item __focusFrameTarget: indicator
    readonly property Item __focusFrameStyleItem: indicator
}
