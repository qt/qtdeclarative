import TestTypes
import QtQml

QtObject {
    id: parent
    property date dayz: BirthdayParty.rsvp
    property QtObject o: QtObject {}

    property date oParty: o.BirthdayParty.rsvp
    Component.onCompleted: {
        BirthdayParty.rsvp = new Date(2121, 0, 12);
        o.BirthdayParty.rsvp = new Date(2111, 11, 11);
    }
}
