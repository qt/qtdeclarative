pragma Strict
import QtQml
import TestTypes as T

QtObject {
    id: self
    property date foo: self.T.BirthdayParty.rsvp
    property date bar: T.BirthdayParty.rsvp
    property string baz: T.CppSingleton.objectName
}
