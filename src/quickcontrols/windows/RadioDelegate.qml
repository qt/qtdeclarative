// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultRadioDelegate {
    contentItem: NativeStyle.DefaultItemDelegateIconLabel {
        color: control.highlighted ? control.palette.button : control.palette.windowText

        readonly property bool __ignoreNotCustomizable: true
    }
}
