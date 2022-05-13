// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.2


Item {

    Component {
        id: mock
        Sample {
        }
    }

    component Mock : Sample {}

    TestCase {
        id: root
        name: "ComponentTest"

        function test_create()
        {
            let dialog = createTemporaryObject(mock, root);
            verify(dialog);
        }
    }
}
