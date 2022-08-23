// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtTest
import QtQuick

Item {
    id: root
    property int finished: 0
    property TestCase currentTest

    Repeater {
        model: 100
        TestCase {
            id: test

            name: "A"

            required property int index
            when: root.children.length > 50 && ((root.finished + index) % 17 == 0 || cutoff.completed)

            function initTestCase() {
                compare(root.currentTest, null)
                root.currentTest = test
            }

            function init() {
                compare(root.currentTest, test)
            }

            function test_do() {
                compare(root.currentTest, test)
                ++root.finished
                wait(index / 10)
                compare(root.currentTest, test)
            }

            function cleanup() {
                compare(root.currentTest, test)
            }

            function cleanupTestCase() {
                compare(root.currentTest, test)
                root.currentTest = null
            }
        }
    }

    TestCase {
        id: cutoff
        name: "C"
        when: root.finished > 80

        function initTestCase() {
            compare(root.currentTest, null)
            root.currentTest = cutoff
        }

        function init() {
            compare(root.currentTest, cutoff)
        }

        function test_cutoff() {
            compare(root.currentTest, cutoff)
        }

        function cleanup() {
            compare(root.currentTest, cutoff)
        }

        function cleanupTestCase() {
            compare(root.currentTest, cutoff)
            root.currentTest = null
        }
    }

    TestCase {
        id: sum
        name: "B"
        when: root.finished === 100

        function initTestCase() {
            compare(root.currentTest, null)
            root.currentTest = sum
        }

        function init() {
            compare(root.currentTest, sum)
        }


        function test_sum() {
            compare(root.currentTest, sum)
            var numTests = 0;
            for (var i in root.children) {
                var test = root.children[i];
                if (test.name === "A") {
                    verify(test.completed)
                    ++numTests
                }
            }

            compare(numTests, 100)
            compare(root.currentTest, sum)
        }


        function cleanup() {
            compare(root.currentTest, sum)
        }

        function cleanupTestCase() {
            compare(root.currentTest, sum)
            root.currentTest = null
        }
    }
}
