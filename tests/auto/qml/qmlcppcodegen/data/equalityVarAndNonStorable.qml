import QtQml
import TestTypes

QtObject {
    id: self
    property var a: 1
    property bool aIsNull: a === null
    property bool aIsNotNull: null !== a
    property bool aIsNotUndefined: a !== undefined

    property var thisObject: self
    property var objectIsNotNull : thisObject !== null

    property list<date> typedArray: []
    property var varTypedArray: typedArray
    property bool typedArrayIsNull: null === varTypedArray

    property var undefinedVar
    property bool isUndefined: undefinedVar === undefined

    // QObject derived
    property var qObjectDerived: Person {name: "patron"}
    property bool derivedIsNull: null === qObjectDerived

    property wrappedWithVariant wrapped
    // QJSValue
    property bool jsValueIsNull: wrapped.nullJsValue === null
    property bool jsValueIsDefined: wrapped.intJSValue !== null &&  undefined !==  wrapped.intJSValue
    property bool jsValueIsUndefined: wrapped.undefinedJsValue === undefined

    // QJSPrimitiveType
    property bool primitiveIsNull: wrapped.nullPrimitiveValue === null
    property bool primitiveIsDefined: wrapped.intPrimitiveValue !== null &&  undefined !==  wrapped.intPrimitiveValue
    property bool primitiveIsUndefined: wrapped.undefinedPrimitiveValue === undefined

    // Non-strict cases
    property var nullVar: null
    property bool nullVarIsUndefined: nullVar == undefined
    property bool nullIsUndefined: null == undefined
    property bool nullVarIsNull: nullVar == null
    property bool nullIsNotUndefined: null !== undefined
}
