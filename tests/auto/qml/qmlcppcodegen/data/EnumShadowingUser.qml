pragma Strict

import QtQuick
import TestTypes

Item  {
    property QtObject mixedDerived: EnumShadowingMixedDerived {
        property int i: EnumShadowingMixedDerived.K
    }

    property QtObject cppDerived:EnumShadowingCppDerived {
        property int i: EnumShadowingCppDerived.K
    }

    property QtObject qmlDerived:EnumShadowingQmlDerived {
        property int i: EnumShadowingQmlDerived.K
    }
}
