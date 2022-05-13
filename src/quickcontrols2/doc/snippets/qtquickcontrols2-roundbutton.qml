// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
RoundButton {
    text: "\u2713" // Unicode Character 'CHECK MARK'
    onClicked: textArea.readOnly = true
}
//! [1]
