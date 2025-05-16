// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultButton {
    id: control
    readonly property Item __focusFrameTarget: control
}
