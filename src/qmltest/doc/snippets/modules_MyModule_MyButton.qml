// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//! [define]
import QtQuick
import QtQuick.Controls

Button {
    width: 50; height: 50
    onClicked: { width = 100; }
}
//! [define]
