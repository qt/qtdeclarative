import QtQuick 2.0

QtObject {
    property bool pBool: true
    property int pInt: 666
    property real pReal: 3.1415927
    property string pString: 'foo'
    property url pUrl: 'http://tools.ietf.org/html/rfc3986#section-1.1.2'
    property color pColor: Qt.rgba(1, 0, 0, 0.5)
    property QtObject pObject: QtObject { property string foo: 'bar' }

    // Test assignment to variant
    property variant pBoolVar: pBool
    property variant pIntVar: pInt
    property variant pRealVar: pReal
    property variant pStringVar: pString
    property variant pUrlVar: pUrl
    property variant pColorVar: pColor
    property variant pObjectVar: pObject
    property variant pNullVar: null
    property variant pVarVar: pUrlVar

    // Test equivalence
    property bool boolConversionSuccess: (pBoolVar == true)
    property bool intConversionSuccess: (pIntVar == 666)
    property bool realConversionSuccess: (pRealVar == 3.1415927)
    property bool stringConversionSuccess: (pStringVar == 'foo')

    property url comparisonUrl: 'http://tools.ietf.org/html/rfc3986#section-1.1.2'
    property bool urlConversionSuccess: (pUrlVar == comparisonUrl)

    property color comparisonColor: Qt.rgba(1, 0, 0, 0.5)
    property bool colorConversionSuccess: (pColorVar == comparisonColor)

    property bool objectConversionSuccess: (pObjectVar == pObject)
    property bool nullConversionSuccess: (pNullVar == null)

    property bool variantConversionSuccess: (pVarVar == comparisonUrl)

    // Operations are not handled by V4 - they should pass through correctly
    property variant pVarNot: !pBoolVar
    property variant pVarComplement: ~pIntVar
    property variant pVarEqual: (pBoolVar == pBoolVar)
    property variant pVarLiteralEqual: (pBoolVar == true)
    property variant pVarUnequal: (pUrlVar == pColorVar)
    property variant pVarComparison: (pIntVar <= pIntVar)
    property variant pVarShift: (pIntVar >> 1)

    Component.onCompleted: {
        if (!boolConversionSuccess) console.warn('QV4: bool conversion failed');
        if (!intConversionSuccess) console.warn('QV4: int conversion failed');
        if (!realConversionSuccess) console.warn('QV4: real conversion failed');
        if (!stringConversionSuccess) console.warn('QV4: string conversion failed');
        if (!urlConversionSuccess) console.warn('QV4: url conversion failed');
        if (!colorConversionSuccess) console.warn('QV4: color conversion failed');
        if (!objectConversionSuccess) console.warn('QV4: object conversion failed');
        if (!nullConversionSuccess) console.warn('QV4: null conversion failed');
        if (!variantConversionSuccess) console.warn('QV4: variant conversion failed');
        if (pVarNot != false) console.warn('QV4: variant negation impeded');
        if (pVarComplement != ~666) console.warn('QV4: variant complement impeded');
        if (pVarEqual != true) console.warn('QV4: variant equality impeded');
        if (pVarLiteralEqual != true) console.warn('QV4: variant/literal equality impeded');
        if (pVarUnequal != false) console.warn('QV4: variant unequality impeded');
        if (pVarComparison != true) console.warn('QV4: variant comparison impeded');
        if (pVarShift != 333) console.warn('QV4: variant shift impeded');
    }
}
