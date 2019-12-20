import Test 1.0
import QtQml 2.0

MyTypeObject {
    property QtObject colorPropertyObject: colorProperty.object
    property string colorPropertyName: colorProperty.name
    property QtObject invalidPropertyObject: invalidProperty.object
    property string invalidPropertyName: invalidProperty.name
}
