// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.2
import QtTest 1.1

Item {
    TestCase {
        name: "URL"
        property url path1: "path1"
        property url path2: "path2"

        function test_url_compare() {
            expectFail("", "compare() should fail here; if it passes we have QTBUG-61297")
            compare(path1, path2)
        }
        function test_url_compare_string() {
            expectFail("", "compare() should fail here")
            compare(path1.toString(), path2.toString())
        }
        function test_url_verify() {
            verify(path1 != path2)
        }
    }
}
