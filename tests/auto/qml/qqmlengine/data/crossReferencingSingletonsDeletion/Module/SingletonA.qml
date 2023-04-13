pragma Singleton
import QtQuick

Item {
    readonly property string name: "SingletonA"

    readonly property TestItem itemA: TestItem{}

    property TestItem crossRef: SingletonB.itemB
    property int testItemInt: crossRef.i
}
