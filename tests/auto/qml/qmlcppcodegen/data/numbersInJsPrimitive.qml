pragma Strict
import QtQml
import TestTypes

QtObject {
    function writeValues() {
        Druggeljug.myInt8 = 35
        Druggeljug.myUint8 = 36
        Druggeljug.myInt16 = 37
        Druggeljug.myUint16 = 38
        Druggeljug.myInt = 39
        Druggeljug.myUint = 40
        Druggeljug.myInt32 = 41
        Druggeljug.myUint32 = 42
    }

    function negateValues() {
        Druggeljug.myInt8   = -Druggeljug.myInt8;
        Druggeljug.myUint8  = -Druggeljug.myUint8;
        Druggeljug.myInt16  = -Druggeljug.myInt16;
        Druggeljug.myUint16 = -Druggeljug.myUint16;
        Druggeljug.myInt    = -Druggeljug.myInt;
        Druggeljug.myUint   = -Druggeljug.myUint;
        Druggeljug.myInt32  = -Druggeljug.myInt32;
        Druggeljug.myUint32 = -Druggeljug.myUint32;
    }

    function shuffleValues() {
        Druggeljug.myInt8   = Druggeljug.myUint8;
        Druggeljug.myUint8  = Druggeljug.myInt16;
        Druggeljug.myInt16  = Druggeljug.myUint16;
        Druggeljug.myUint16 = Druggeljug.myInt;
        Druggeljug.myInt    = Druggeljug.myUint;
        Druggeljug.myUint   = Druggeljug.myInt32;
        Druggeljug.myInt32  = Druggeljug.myUint32;
        Druggeljug.myUint32 = Druggeljug.myInt8;
    }

    function readValueAsString(i: int) : string {
        switch (i) {
            case 0: return Druggeljug.myInt8;
            case 1: return Druggeljug.myUint8;
            case 2: return Druggeljug.myInt16;
            case 3: return Druggeljug.myUint16;
            case 4: return Druggeljug.myInt;
            case 5: return Druggeljug.myUint;
            case 6: return Druggeljug.myInt32;
            case 7: return Druggeljug.myUint32;
            default: return "";
        }
    }

    function storeValues() {
        Druggeljug.storeMyInt8(1330)
        Druggeljug.storeMyUint8(1331)
        Druggeljug.storeMyInt16(1332)
        Druggeljug.storeMyUint16(1333)
        Druggeljug.storeMyInt(1334)
        Druggeljug.storeMyUint(1335)
        Druggeljug.storeMyInt32(1336)
        Druggeljug.storeMyUint32(1337)
    }
}
