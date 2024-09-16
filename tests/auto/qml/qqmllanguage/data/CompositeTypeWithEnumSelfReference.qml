import QtQml
import Test

QtObject {
    enum A { B, C, D }
    property int e: CompositeTypeWithEnumSelfReference.C
    property int f: CompositeTypeWithEnumSelfReference.A.D
}
