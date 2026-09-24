import QtQml
import AutoImportTarget 2.0

QtObject {
    id: root
    property real num: 5
    property QtObject thing: Thing {
        value: root.num / 2
    }
}
