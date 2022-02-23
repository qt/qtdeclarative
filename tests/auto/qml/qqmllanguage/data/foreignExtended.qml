import QtQml 2.12
import Test 1.0

QtObject {
    property Foreign foreign: Foreign {
        objectName: "foreign"
    }
    property Extended extended: Extended {
        objectName: "extended"
        property int changeCount: 0
        onExtensionChanged: ++changeCount
    }
    property ForeignExtended foreignExtended: ForeignExtended {
        objectName: "foreignExtended"
    }

    property int extendedBase: extended.base
    property int extendedChangeCount: extended.changeCount

    property int extendedInvokable: extended.invokable()
    property int extendedSlot: extended.slot()

    property int extendedExtension: extended.extension
    property int foreignExtendedExtension: foreignExtended.extension

    property string foreignObjectName: foreign.objectName
    property string foreignExtendedObjectName: foreignExtended.objectName
}
