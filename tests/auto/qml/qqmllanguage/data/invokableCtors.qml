import QtQml as QQ
import Test as VV

QQ.QtObject {
    property QQ.QtObject oo: new QQ.QtObject()
    property QQ.QtObject pp: new QQ.QtObject(oo)
    property VV.vv v: new VV.vv("green")

    property VV.InvokableSingleton i: new VV.InvokableSingleton(5, oo)
    property VV.InvokableExtended k: new VV.InvokableExtended()
    property VV.InvokableUncreatable l: new VV.InvokableUncreatable()
}
