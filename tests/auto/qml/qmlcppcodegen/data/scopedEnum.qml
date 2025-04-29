import QtQml
import TestTypes

QtObject {
    property int good: Data.DType.A
    property int bad: Data.A

    property int wrong: Data.EType.C
    property int right: Data.C

    property int notgood: Data2.DType.A
    property int notbad: Data2.A

    property int notwrong: Data2.EType.C
    property int notright: Data2.C

    property int passable: Enums.AppState.Blue
    property int wild: Enums.Green
}
