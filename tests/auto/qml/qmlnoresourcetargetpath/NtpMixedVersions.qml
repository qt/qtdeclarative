pragma Strict
import QtQml
import NtpVersioned 1.0
import NtpVersioned 2.0 as V

QtObject {
    property V.Late late: V.Late {}
    property real lateLife: late.life

    function poke(): void { late.life = 1 }
}
