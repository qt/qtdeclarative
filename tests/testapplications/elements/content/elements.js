// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

var newcomponent = Qt.createComponent("AppContainer.qml");
var appfile;
var demoapp;


function setapp(appname,par) {
    appfile = appname;
    demoapp = newcomponent.createObject(par);
    demoapp.qmlfile = appfile;
}


function removeApp() {
    demoapp.destroy();
}
