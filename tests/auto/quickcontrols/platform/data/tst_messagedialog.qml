// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import Qt.labs.platform

TestCase {
    id: testCase
    width: 200
    height: 200
    name: "MessageDialog"

    Component {
        id: messageDialog
        MessageDialog { }
    }

    SignalSpy {
        id: spy
    }

    function test_instance() {
        var dialog = messageDialog.createObject(testCase)
        verify(dialog)
        dialog.destroy()
    }

    function test_standardButtons() {
        // Q_FLAGS(QPlatformDialogHelper::StandardButtons)
        compare(MessageDialog.NoButton       , 0x00000000)
        compare(MessageDialog.Ok             , 0x00000400)
        compare(MessageDialog.Save           , 0x00000800)
        compare(MessageDialog.SaveAll        , 0x00001000)
        compare(MessageDialog.Open           , 0x00002000)
        compare(MessageDialog.Yes            , 0x00004000)
        compare(MessageDialog.YesToAll       , 0x00008000)
        compare(MessageDialog.No             , 0x00010000)
        compare(MessageDialog.NoToAll        , 0x00020000)
        compare(MessageDialog.Abort          , 0x00040000)
        compare(MessageDialog.Retry          , 0x00080000)
        compare(MessageDialog.Ignore         , 0x00100000)
        compare(MessageDialog.Close          , 0x00200000)
        compare(MessageDialog.Cancel         , 0x00400000)
        compare(MessageDialog.Discard        , 0x00800000)
        compare(MessageDialog.Help           , 0x01000000)
        compare(MessageDialog.Apply          , 0x02000000)
        compare(MessageDialog.Reset          , 0x04000000)
        compare(MessageDialog.RestoreDefaults, 0x08000000)
    }

    function test_clicked() {
        var dialog = messageDialog.createObject(testCase)
        verify(dialog)

        spy.target = dialog
        spy.signalName = "clicked"

        dialog.clicked(MessageDialog.Yes)
        compare(spy.count, 1)
        compare(spy.signalArguments[0][0], MessageDialog.Yes)

        spy.clear()
        dialog.destroy()
    }
}
