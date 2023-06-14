import QtQml
import People

QtObject {
    id: root

    property QtObject b: QtObject { id: g1; objectName: "Leo Hodges" }
    property QtObject c: QtObject { id: g2; objectName: "Jack Smith" }
    property QtObject d: QtObject { id: g3; objectName: "Anne Brown" }

    property Component pc: Component {
        id: partyComp
        BirthdayParty {}
    }

    property BirthdayParty q: partyComp.createObject(root, { guests: [g1, g2, g3] })
}
