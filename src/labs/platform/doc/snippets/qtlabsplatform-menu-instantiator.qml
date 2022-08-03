// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import Qt.labs.platform

//! [menu]
Menu {
    title: qsTr("File")

    Menu {
        id: recentFilesMenu
        title: qsTr("Recent Files")
        enabled: recentFilesInstantiator.count > 0

        Instantiator {
            id: recentFilesInstantiator
            model: settings.recentFiles
            delegate: MenuItem {
                text: settings.displayableFilePath(modelData)
                onTriggered: loadFile(modelData)
            }

            onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Clear Recent Files")
            onTriggered: settings.clearRecentFiles()
        }
    }
}
//! [menu]
