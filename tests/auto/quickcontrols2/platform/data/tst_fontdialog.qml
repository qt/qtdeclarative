// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import Qt.labs.platform

TestCase {
    id: testCase
    width: 200
    height: 200
    name: "FontDialog"

    Component {
        id: fontDialog
        FontDialog { }
    }

    function test_instance() {
        var dialog = fontDialog.createObject(testCase)
        verify(dialog)
        dialog.destroy()
    }
}
