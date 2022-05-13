// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1

Item {
  TestCase {
    name:"data-driven-empty-init-data"
    property int tests:0;
    property int init_data_called_times:0;
    function init_data() {init_data_called_times++;}
    function initTestCase() {tests = 0; init_data_called_times = 0;}
    function cleanupTestCase() {compare(tests, 2); compare(init_data_called_times, 2);}

    function test_test1() {tests++;}
    function test_test2() {tests++;}
  }
  TestCase {
    name:"data-driven-no-init-data"
    property int tests:0;
    function initTestCase() {tests = 0;}
    function cleanupTestCase() {compare(tests, 2);}

    function test_test1() {tests++;}
    function test_test2() {tests++;}
  }
  TestCase {
    name:"data-driven-init-data"
    property int tests:0;
    property int init_data_called_times:0;
    function initTestCase() {tests = 0; init_data_called_times = 0;}
    function cleanupTestCase() {compare(tests, 2); compare(init_data_called_times, 1);}
    function init_data() {init_data_called_times++; return [{tag:"data1", data:"test data 1"}];}

    function test_test1(row) {tests++; compare(row.data, "test data 1");}
    function test_test2_data() {return [{tag:"data2", data:"test data 2"}]; }
    function test_test2(row) {tests++; compare(row.data, "test data 2");}
  }
}
