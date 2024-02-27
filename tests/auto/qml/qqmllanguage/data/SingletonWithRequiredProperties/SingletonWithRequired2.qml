pragma Singleton
import QtQml

QtObject {
    property QtObject o: QtObject {
        required property int i
    }
}
