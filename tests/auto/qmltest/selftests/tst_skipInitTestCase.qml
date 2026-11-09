// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtTest
import QtQuick

TestCase {
    name: "SkipInitTestCase"

    function initTestCase() {
        skip("Skipping initTestCase() skips all test functions")
    }

    function test_first() {
        fail("test_first() ran even though initTestCase() was skipped")
    }

    function test_second() {
        fail("test_second() ran even though initTestCase() was skipped")
    }
}
