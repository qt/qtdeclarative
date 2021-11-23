import TestTypes
import QtQml

QtObject {
    id: parent
    property int a: 16
    property date dayz: BirthdayParty.rsvp
    property QtObject o: QtObject {
        property date width: parent.dayz
    }

    property date oParty: o.BirthdayParty.rsvp

    property BirthdayParty party: BirthdayParty {
        eee: parent.a + 5
        property int fff: eee + 12
    }

    property int ggg: party.eee + a
}
