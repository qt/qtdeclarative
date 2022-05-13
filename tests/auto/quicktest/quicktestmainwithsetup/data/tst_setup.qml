// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.2

import QmlRegisterTypeCppModule 1.0
import ImportPathQmlModule 1.0

TestCase {
    name: "setup"

    QmlRegisterTypeCppType {}
    ImportPathQmlType {}

    function initTestCase()
    {
        verify(qmlEngineAvailableCalled)
    }
}
