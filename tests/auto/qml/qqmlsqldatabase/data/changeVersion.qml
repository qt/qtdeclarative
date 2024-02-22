// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.2
import QtQuick.LocalStorage 2.0

QtObject {
    property var db;
    property string version;

    function create() {
        db = LocalStorage.openDatabaseSync("testdb", "2", "stuff for testing", 100000);
        version = db.version;
    }

    function upgrade() {
        db = db.changeVersion(db.version, "22", function(y) {});
        version = db.version;
    }
}
