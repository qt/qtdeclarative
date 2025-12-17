// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultFrame {
    leftPadding: 9 + (__nativeBackground ? background.contentPadding.left : 0)
    rightPadding: 9 + (__nativeBackground ? background.contentPadding.right : 0)
    topPadding: 9 + (__nativeBackground ? background.contentPadding.top : 0)
    bottomPadding: 9 + (__nativeBackground ? background.contentPadding.bottom : 0)
}
