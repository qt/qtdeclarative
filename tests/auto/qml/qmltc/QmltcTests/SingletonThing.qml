pragma Singleton
import QtQml

QtObject {
    property int integerProperty: 42
    property string stringProperty: "hello"
    enum MyEnum {
        Normal,
        Special
    }
}
