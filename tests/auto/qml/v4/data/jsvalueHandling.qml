import QtQuick 2.0
import Qt.v4 1.0

JSValueTest {
    property bool pBool: true
    property int pInt: 666
    property real pReal: 3.1415927
    property string pString: 'foo'
    property url pUrl: 'http://tools.ietf.org/html/rfc3986#section-1.1.2'
    property color pColor: Qt.rgba(1, 0, 0, 0.5)
    property QtObject pObject: QtObject { property string foo: 'bar' }
    property var pVar: pUrl

    // Test assignment to QJSValue
    boolVar: pBool
    intVar: pInt
    realVar: pReal
    stringVar: pString
    urlVar: pUrl
    colorVar: pColor
    objectVar: pObject
    nullVar: null
    varVar: pVar

    // Test equivalence
    property bool boolConversionSuccess: (boolVar == true)
    property bool intConversionSuccess: (intVar == 666)
    property bool realConversionSuccess: (realVar == 3.1415927)
    property bool stringConversionSuccess: (stringVar == 'foo')

    property url comparisonUrl: 'http://tools.ietf.org/html/rfc3986#section-1.1.2'
    property bool urlConversionSuccess: (urlVar == comparisonUrl)

    property color comparisonColor: Qt.rgba(1, 0, 0, 0.5)
    property bool colorConversionSuccess: (colorVar == comparisonColor)

    property bool objectConversionSuccess: (objectVar == pObject)
    property bool nullConversionSuccess: (nullVar == null)

    property bool varConversionSuccess: (varVar == comparisonUrl)

    // Operations are not handled by V4 - they should pass through correctly
    property var pVarNot: !boolVar
    property var pVarComplement: ~intVar
    property var pVarEqual: (boolVar == pBool)
    property var pVarLiteralEqual: (boolVar == true)
    property var pVarUnequal: (urlVar == colorVar)
    property var pVarComparison: (intVar <= intVar)
    property var pVarShift: (intVar >> 1)

    Component.onCompleted: {
        if (!boolConversionSuccess) console.warn('QV4: bool conversion failed');
        if (!intConversionSuccess) console.warn('QV4: int conversion failed');
        if (!realConversionSuccess) console.warn('QV4: real conversion failed');
        if (!stringConversionSuccess) console.warn('QV4: string conversion failed');
        if (!urlConversionSuccess) console.warn('QV4: url conversion failed');
        if (!colorConversionSuccess) console.warn('QV4: color conversion failed');
        if (!objectConversionSuccess) console.warn('QV4: object conversion failed');
        if (!nullConversionSuccess) console.warn('QV4: null conversion failed');
        if (!varConversionSuccess) console.warn('QV4: var conversion failed');
        if (pVarNot != false) console.warn('QV4: var negation impeded');
        if (pVarComplement != ~666) console.warn('QV4: var complement impeded');
        if (pVarEqual != true) console.warn('QV4: var equality impeded');
        if (pVarLiteralEqual != true) console.warn('QV4: var/literal equality impeded');
        if (pVarUnequal != false) console.warn('QV4: var unequality impeded');
        if (pVarComparison != true) console.warn('QV4: var comparison impeded');
        if (pVarShift != 333) console.warn('QV4: var shift impeded');
    }
}
