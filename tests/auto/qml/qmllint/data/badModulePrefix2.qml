import QtQml
import TestTypes as T

QtObject {
    property rect rr
    property date foo: rr.T.BirthdayParty.rsvp
}
