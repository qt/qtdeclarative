pragma Strict
import QtQml
import TestTypes

QtObject {
    function writeValues() {
        Druggeljug.myInt = 39
        Druggeljug.myUint = 40
        Druggeljug.myInt32 = 41
        Druggeljug.myUint32 = 42
    }

    function readValueAsString(i: int) : string {
        switch (i) {
            case 0: return Druggeljug.myInt;
            case 1: return Druggeljug.myUint;
            case 2: return Druggeljug.myInt32;
            case 3: return Druggeljug.myUint32;
            default: return "";
        }
    }

    function storeValues() {
        Druggeljug.storeMyInt(1334)
        Druggeljug.storeMyUint(1335)
        Druggeljug.storeMyInt32(1336)
        Druggeljug.storeMyUint32(1337)
    }
}
