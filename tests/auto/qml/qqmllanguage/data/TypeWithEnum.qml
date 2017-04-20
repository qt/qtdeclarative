import QtQuick 2.0

QtObject {
    enum MyEnum {
        EnumValue1,
        EnumValue2,
        EnumValue3
    }

    property int enumValue: TypeWithEnum.EnumValue2
    property int enumValue2
    property int scopedEnumValue: TypeWithEnum.MyEnum.EnumValue2
    Component.onCompleted: enumValue2 = TypeWithEnum.EnumValue3
}
