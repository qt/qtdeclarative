// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    id: root
    property bool patronChanged // shouldn't be found

    Item {
        id: inner
        property bool patronChanged

        Binding on patronChanged {
            value: !inner.patronChanged
            when: root.patronChanged // // shouldn't be found
        }

        Binding {
            target: inner
            property: "patronChanged"
            value: !inner.patronChanged
            when: root.patronChanged // shouldn't be found
        }

        // generalized dot
        Binding {
            inner.patronChanged: !inner.patronChanged
            when: root.patronChanged // // shouldn't be found
        }

        // generalized block
        Binding {
            inner {
                patronChanged: false
            }
            when: root.patronChanged // // shouldn't be found
        }
    }
}
