// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int p2: 1

    component NestedComponent: Item {
        property NestedComponent2 inner: NestedComponent2 {}
        property int p2
    }
    component NestedComponent2: Item {
        property NestedComponent3 inner
        property int p2
        inner: NestedComponent3 {}
    }
    component NestedComponent3: Item {
        property NestedComponent4 inner
        property int p2
        inner: NestedComponent4 {}

    }
    component NestedComponent4: Item {
        property int helloProperty
        property int p2
    }
    NestedComponent {
        id: myNested
    }
    function nestedUsages() {
        let x = myNested.inner.inner.inner.helloProperty;
        let a = myNested.p2 + p2
        let b = myNested.inner.p2 + p2
        let c = myNested.inner.inner.p2 + p2
        let d = myNested.inner.inner.inner.p2 + p2
    }

    function f() {
        {
            let _p2 = 34;
            return _p2 + p2
        }
    }

    NestedComponentInFile {
        id: myNestedInFile
    }
    function nestedUsagesInFile() {
        let x = myNestedInFile.inner.inner.inner.helloProperty;
        let a = myNestedInFile.p2
        let b = myNestedInFile.inner.p2
        let c = myNestedInFile.inner.inner.p2
        let d = myNestedInFile.inner.inner.inner.p2
    }
}