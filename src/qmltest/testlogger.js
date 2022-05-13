// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

.pragma library

var testResults = null;

function log_init_results()
{
    if (!testResults) {
        testResults = {
            reportedStart: false,
            nextId: 0,
            testCases: []
        }
    }
}

function log_register_test(name)
{
    log_init_results()
    var testId = testResults.nextId++
    testResults.testCases.push(testId)
    return testId
}

function log_optional_test(testId)
{
    log_init_results()
    var index = testResults.testCases.indexOf(testId)
    if (index >= 0)
        testResults.testCases.splice(index, 1)
}

function log_mandatory_test(testId)
{
    log_init_results()
    var index = testResults.testCases.indexOf(testId)
    if (index == -1)
        testResults.testCases.push(testId)
}

function log_start_test()
{
    log_init_results()
    if (testResults.reportedStart)
        return false
    testResults.reportedStart = true
    return true
}

function log_complete_test(testId)
{
    var index = testResults.testCases.indexOf(testId)
    if (index >= 0)
        testResults.testCases.splice(index, 1)
    return testResults.testCases.length > 0
}
