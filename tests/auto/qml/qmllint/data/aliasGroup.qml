import QtQml

QtObject {
    id: root

    property QtObject o: SampleDataStoreManagerSettingsSection {
        contentControl.contentItem: QtObject { objectName: "a" }
    }

    component SampleDataStoreManagerSettingsSection: QtObject {
        property alias contentControl: sectionContentItem

        property QtObject o: QtObject {
            id: sectionContentItem
            property QtObject contentItem

        }
    }
}

