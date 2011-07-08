import QtQuick 1.0

QtObject {
    // months are 0-based - so January = 0, December = 11.
    property variant qtime: new Date(0,0,0,14,15,38,200)            // yyyy/MM/dd 14:15:38.200
    property variant qdate: new Date(2008,11,24)                    // 2008/12/24 hh:mm:ss.zzz
    property variant qdatetime: new Date(2008,11,24,14,15,38,200)   // 2008/12/24 14:15:38.200

    property variant qdatetime2: new Date(2852,11,31,23,59,59,500)  // 2852/12/31 23:59:59.500
    property variant qdatetime3: new Date(1970,0,1,0,0,0,0)         // 1970/01/01 00:00:00.000
    property variant qdatetime4: new Date(1586,1,2)                 // 1586/02/02 hh:mm:ss.zzz
    property variant qdatetime5: new Date(955,0,1,0,0,0,0)          //  955/01/01 00:00:00.000
    property variant qdatetime6: new Date(113,1,24,14,15,38,200)    //  113/02/24 14:15:38.200
}
