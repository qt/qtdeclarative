pragma Strict
import QtQml
import TestTypes

QtObject {
    property var d
    property var r
    property var v

    property Person person: Person { id: person }

    Component.onCompleted: {
        BirthdayParty.rsvp = new Date(1997, 2, 3, 4, 5);
        d = BirthdayParty.rsvp
        BirthdayParty.rsvp = new Date(2001, 5);

        person.area = { x: 2, y: 3, width: 4, height: 5};
        r = person.area;
        person.area.x = 22;

        person.things = ["a", 3, new Date(), this, null];
        v = person.things;
        person.things[0] = "c";
    }
}
