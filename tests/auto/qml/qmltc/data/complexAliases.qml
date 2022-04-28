import QtQuick
Item {
    id: root
    property alias aText: theRect.text // new style property
    property alias aColor: theText.color // old style property
    property alias aRectObject: theRect
    property alias aTextObject: theText
    property alias aLetterSpacing: theText.font.letterSpacing
    property alias aWordSpacing: theText.font.wordSpacing

    property alias aFont: theText.font

    property alias aliasToObjectAlias: root.aRectObject
    property alias aliasToPropertyAlias: root.aText
    property alias aliasToValueTypeAlias: root.aFont
    property alias aliasToPropertyOfValueTypeAlias: root.aFont.pixelSize

    property alias aliasToImportedMessage: localImport.message

    property alias aliasToOnAssignmentProperty: accessibleBehavior.targetProperty.name

    property alias aliasToPrivatePalette: theRect.palette
    property alias aliasToAnchors: theRect.anchors
    // Note: cannot alias anchors.<property> - rejected by QQmlComponent
    // property alias aliasToAnchorsAlignWhenCentered: theRect.anchors.alignWhenCentered

    property alias aliasToPrivateData: root.data

    Rectangle {
        id: theRect
        property string text: "whatever"
        property alias aliasToInternalImportedX: theRectInsideImported.x

        Text {
            id: theText
            color: "red"
            font.pixelSize: color == "red" ? 11 : 12
            Behavior on width {
                id: accessibleBehavior
                NumberAnimation { duration: 1000 }
            }
        }
    }

    LocallyImported {
        id: localImport
        property alias importedAliasToText: theRect.text

        Rectangle {
            id: theRectInsideImported
            property alias internallyImportedAliasToText: theText.text
        }
    }

    function updateTextThroughAlias(newValue: string)
    {
        aText = newValue;
    }

    function updateText(newValue: string)
    {
        theRect.text = newValue;
    }
}
