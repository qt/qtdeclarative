// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Start Test
// Open that test.qml file in a text editor that has a client for qmlls

////////////////////////////////////////////////////////////
// Test Case - 1
// Type the following line manually

import QtQml

Window {


}

// Then, execute "Format Document" in the text editor

// Expected Result
import QtQml

Window {
}

////////////////////////////////////////////////////////////
// Test Case - 2
// Type the following line manually

import QtQml

Window {
    aa a: {
    }
}

// Then, execute "Format Document" in the text editor

// Expected Result
import QtQml

Window {
    aa: {
    }
}
