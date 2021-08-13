import QtQml
import Test

QtObject {
    id: objectName
    property int width: 12

    property QtObject child: ImmediateProperties {
        objectName.width: 14
    }
}
