// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

function basic() {
    return qsTr("hello")
}

function basic2() {
    return qsTranslate("CustomContext", "goodbye")
}

function basic3() {
    if (1)
        return qsTr("hello")
    return "";
}

function disambiguation() {
    return qsTr("hi", "informal 'hello'")
}

function disambiguation2() {
    return qsTranslate("CustomContext", "see ya", "informal 'goodbye'")
}

function disambiguation3() {
    if (1)
        return qsTr("hi", "informal 'hello'")
    return "";
}

function noop() {
    var _noop = QT_TR_NOOP("hello")
    return qsTr(_noop)
}

function noop2() {
    var _noop2 = QT_TRANSLATE_NOOP("CustomContext", "goodbye")
    return qsTranslate("CustomContext", _noop2)
}

function singular() {
    return qsTr("%n duck(s)", "", 1)
}

function singular2() {
    if (1)
        return qsTr("%n duck(s)", "", 1)
    return "";
}

function plural() {
    return qsTr("%n duck(s)", "", 2)
}

function plural2() {
    if (1)
        return qsTr("%n duck(s)", "", 2)
    return "";
}

function emptyContext() {
    return qsTranslate("", "hello")
}
