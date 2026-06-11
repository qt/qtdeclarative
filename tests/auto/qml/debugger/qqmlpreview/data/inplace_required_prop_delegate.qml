// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window

// Used by tst_QQmlPreview::inPlaceRequiredPropertyDelegateCrash.
//
// Changing clip: true → false triggers a rebuildObject() on the ListView.
// stashExternalState() discovers the currentItem (an inline delegate whose
// compilationUnit matches the file's CU) via the QObject* currentItem property
// and creates a child context for it. The delegate has required properties
// (label, val) for which QQmlDelegateModel installed QQmlPropertyToPropertyBindings
// whose sourceObject is a QQmlDMListAccessorData. reset() then writes null to
// the model property, which causes QQmlDelegateModel to release all items and
// immediately delete the QQmlDMListAccessorData objects (ref-count → 0, not
// via deleteLater). restoreExternalState() later tries to re-install the stashed
// bindings, calling readSourceValue() with the now-dangling sourceObject → crash.
Window {
    visible: true
    width: 200
    height: 200

    ListView {
        anchors.fill: parent
        clip: true
        model: ListModel {
            ListElement { label: "A"; val: 1 }
            ListElement { label: "B"; val: 2 }
            ListElement { label: "C"; val: 3 }
        }
        delegate: Item {
            required property string label
            required property int val
            width: ListView.view.width
            height: 50
        }
    }

    Timer {
        repeat: true
        interval: 200
        running: true
        onTriggered: console.log("required_prop_delegate ready")
    }
}
