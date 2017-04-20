import QtQuick 2.0

QtObject {
    property int enumValue: TypeWithEnum.EnumValue2
    property int enumValue2: -1
    property int scopedEnumValue: TypeWithEnum.MyEnum.EnumValue3
    Component.onCompleted: enumValue2 = TypeWithEnum.EnumValue1
}
