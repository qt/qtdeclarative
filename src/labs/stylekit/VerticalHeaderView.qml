// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.VerticalHeaderView {
    id: control

    implicitWidth: Math.max(1, contentWidth)
    implicitHeight: syncView ? syncView.height : 0

    delegate: VerticalHeaderViewDelegate { }
}
