pragma Strict
import QtQml
import TestTypes

QtObject {
    property DerivedGadgetProvider provider: DerivedGadgetProvider {}

    property int a: provider.value.a               // inherited from baseGadget
    property int b: provider.value.b                // declared by derivedGadget
    property string c: provider.value.c             // declared by derivedGadget

    property int baseMethodResult: provider.value.baseMethod()       // inherited
    property int derivedMethodResult: provider.value.derivedMethod() // declared by derivedGadget
}
