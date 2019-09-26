import QtQml 2.12
import Test 1.0

QtObject {
    property Foreign foreign: Foreign {
        objectName: "foreign"
    }
    property Extended extended: Extended {}
    property ForeignExtended foreignExtended: ForeignExtended {
        objectName: "foreignExtended"
    }

    property int extendedBase: extended.base

    property int extendedExtension: extended.extension
    property int foreignExtendedExtension: foreignExtended.extension

    property string foreignObjectName: foreign.objectName
    property string foreignExtendedObjectName: foreignExtended.objectName
}
