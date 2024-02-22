// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import Qt.labs.platform

TestCase {
    id: testCase
    name: "StandardPaths"

    function test_standardLocation() {
        // Q_ENUMS(QStandardPaths::StandardLocation)
        compare(StandardPaths.DesktopLocation, 0)
        compare(StandardPaths.DocumentsLocation, 1)
        compare(StandardPaths.FontsLocation, 2)
        compare(StandardPaths.ApplicationsLocation, 3)
        compare(StandardPaths.MusicLocation, 4)
        compare(StandardPaths.MoviesLocation, 5)
        compare(StandardPaths.PicturesLocation, 6)
        compare(StandardPaths.TempLocation, 7)
        compare(StandardPaths.HomeLocation, 8)
        compare(StandardPaths.AppLocalDataLocation, 9)
        compare(StandardPaths.CacheLocation, 10)
        compare(StandardPaths.GenericDataLocation, 11)
        compare(StandardPaths.RuntimeLocation, 12)
        compare(StandardPaths.ConfigLocation, 13)
        compare(StandardPaths.DownloadLocation, 14)
        compare(StandardPaths.GenericCacheLocation, 15)
        compare(StandardPaths.GenericConfigLocation, 16)
        compare(StandardPaths.AppDataLocation, 17)
        compare(StandardPaths.AppConfigLocation, 18)
    }

    function test_locateOptions() {
        // Q_ENUMS(QStandardPaths::LocateOptions)
        compare(StandardPaths.LocateFile, 0)
        compare(StandardPaths.LocateDirectory, 1)
    }
}
