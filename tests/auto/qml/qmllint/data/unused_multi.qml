// This is the one kind of unused import we currently do not warn about:
// If two imports provide the same type, one of them could be considered unused.
// We do not however warn here because there might be legitimate reasons why a user would want this.
import QtQuick
import QtQml

QtObject {}
