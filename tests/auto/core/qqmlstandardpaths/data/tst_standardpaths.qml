// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtCore

QtObject {
    // Q_ENUM(QStandardPaths::StandardLocation)
    property int desktop: StandardPaths.DesktopLocation
    property int documents: StandardPaths.DocumentsLocation
    property int fonts: StandardPaths.FontsLocation
    property int applications: StandardPaths.ApplicationsLocation
    property int music: StandardPaths.MusicLocation
    property int movies: StandardPaths.MoviesLocation
    property int pictures: StandardPaths.PicturesLocation
    property int temp: StandardPaths.TempLocation
    property int home: StandardPaths.HomeLocation
    property int appLocalData: StandardPaths.AppLocalDataLocation
    property int cache: StandardPaths.CacheLocation
    property int genericData: StandardPaths.GenericDataLocation
    property int runtime: StandardPaths.RuntimeLocation
    property int config: StandardPaths.ConfigLocation
    property int download: StandardPaths.DownloadLocation
    property int genericCache: StandardPaths.GenericCacheLocation
    property int genericConfig: StandardPaths.GenericConfigLocation
    property int appData: StandardPaths.AppDataLocation
    property int appConfig: StandardPaths.AppConfigLocation

    // Q_ENUMS(QStandardPaths::LocateOptions)
    property int locateFile: StandardPaths.LocateFile
    property int locateDirectory: StandardPaths.LocateDirectory
}
