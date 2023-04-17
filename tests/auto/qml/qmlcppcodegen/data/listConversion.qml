pragma Strict
import QtQml
import TestTypes

BirthdayParty {
    id: self

    guests: [
        Person { name: "Horst 1" },
        Person { name: "Horst 2" },
        Person { name: "Horst 3" }
    ]

    property list<QtObject> o: self.guests
    property list<string> s: self.guestNames
    property list<var> v: self.stuffs
}
