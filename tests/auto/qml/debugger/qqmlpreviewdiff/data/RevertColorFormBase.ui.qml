// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Analog of CustomButtonForm.ui.qml: a form with a defaulted string property
// and a signal an enclosing component can attach a handler to.
import QtQuick

Item {
    id: base
    property string col: "grey"
    signal activated()
}
