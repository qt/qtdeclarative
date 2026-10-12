import QtQml
import qqmllanguage.Test

QtObject {
    id: objectName
    property int width: 12

    property QtObject child: ImmediateProperties {
        objectName.width: 14
    }
}
