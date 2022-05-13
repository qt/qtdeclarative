// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12

Item {
    id: root

    LoggingCategory {
        id: testCategory
        name: "qt.test"
    }

    LoggingCategory {
        id: testCategoryStartingFromWarning
        name: "qt.test.warning"
        defaultLogLevel: LoggingCategory.Warning
    }

    LoggingCategory {
        id: emptyCategory
    }

    Component.onCompleted: {
        console.debug(testCategory, "console.debug");
        console.log(testCategory, "console.log");
        console.info(testCategory, "console.info");
        console.warn(testCategory, "console.warn");
        console.error(testCategory, "console.error");
        console.debug(testCategoryStartingFromWarning, "console.debug");
        console.log(testCategoryStartingFromWarning, "console.log");
        console.info(testCategoryStartingFromWarning, "console.info");
        console.warn(testCategoryStartingFromWarning, "console.warn");
        console.error(testCategoryStartingFromWarning, "console.error");

        testCategory.name = "qt.test"; // should be silent
        testCategoryStartingFromWarning.name = "qt.test.other"; // should issue a warning

        testCategory.defaultLogLevel = LoggingCategory.Debug; // should be silent
        testCategoryStartingFromWarning.defaultLogLevel = LoggingCategory.Debug; // should issue a warning

        console.error(emptyCategory, "console.error");
    }
}
