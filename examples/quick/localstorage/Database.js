// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

function dbInit()
{
    let db = LocalStorage.openDatabaseSync("Activity_Tracker_DB", "", "Track exercise", 1000000)
    try {
        db.transaction(function (tx) {
            tx.executeSql('CREATE TABLE IF NOT EXISTS trip_log (date text,trip_desc text,distance numeric)')
        })
    } catch (err) {
        console.log("Error creating table in database: " + err)
    };
}

function dbGetHandle()
{
    try {
        var db = LocalStorage.openDatabaseSync("Activity_Tracker_DB", "",
                                               "Track exercise", 1000000)
    } catch (err) {
        console.log("Error opening database: " + err)
    }
    return db
}

function dbInsert(Pdate, Pdesc, Pdistance)
{
    let db = dbGetHandle()
    let rowid = 0;
    db.transaction(function (tx) {
        tx.executeSql('INSERT INTO trip_log VALUES(?, ?, ?)',
                      [Pdate, Pdesc, Pdistance])
        let result = tx.executeSql('SELECT last_insert_rowid()')
        rowid = result.insertId
    })
    return rowid;
}

function dbReadAll()
{
    let db = dbGetHandle()
    db.transaction(function (tx) {
        let results = tx.executeSql(
                'SELECT rowid,date,trip_desc,distance FROM trip_log order by rowid desc')
        for (let i = 0; i < results.rows.length; i++) {
            listModel.append({
                                 id: results.rows.item(i).rowid,
                                 checked: " ",
                                 date: results.rows.item(i).date,
                                 trip_desc: results.rows.item(i).trip_desc,
                                 distance: results.rows.item(i).distance
                             })
        }
    })
}

function dbUpdate(Pdate, Pdesc, Pdistance, Prowid)
{
    let db = dbGetHandle()
    db.transaction(function (tx) {
        tx.executeSql(
                    'update trip_log set date=?, trip_desc=?, distance=? where rowid = ?', [Pdate, Pdesc, Pdistance, Prowid])
    })
}

function dbDeleteRow(Prowid)
{
    let db = dbGetHandle()
    db.transaction(function (tx) {
        tx.executeSql('delete from trip_log where rowid = ?', [Prowid])
    })
}
