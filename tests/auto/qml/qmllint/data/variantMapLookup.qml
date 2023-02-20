pragma Strict
import TestTypes
import QtQuick

Item {
    VariantMapLookupFoo {
        id: moo
    }
    Component.onCompleted: {
        moo.data.value = 5
        moo.data["value"] = 6
    }
}
