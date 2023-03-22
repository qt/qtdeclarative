pragma Singleton
import QtQuick

Item {
    readonly property string name: "SingletonB"

    readonly property TestItem itemB: TestItem{}

    property TestItem crossRef: SingletonA.itemA
    property int testItemInt: crossRef.i
}
