pragma Strict
import QtQml

QtObject {
    property date now: new Date()
    property date now2: new Date(now)
    property date fromString: new Date("1995-12-17T03:24:00")
    property date fromNumber: new Date(777)
    property date fromPrimitive: new Date(objectName.length === 0 ? 57 : "1997-02-13T13:04:12")
    property date from2: new Date(1996, 1)
    property date from3: new Date(1996, 2, 3)
    property date from4: new Date(1996, 3, 4, 5)
    property date from5: new Date(1996, 4, 5, 6, 7)
    property date from6: new Date(1996, 5, 6, 7, 8, 9)
    property date from7: new Date(1996, 6, 7, 8, 9, 10, 11)
    property date from8: new Date(1996, 7, 8, 9, 10, 11, 12, 13)

    property date withUnderflow: new Date(-4, -5, -6, -7, -8, -9, -10)
    property date invalid: new Date("foo", "bar")
}
