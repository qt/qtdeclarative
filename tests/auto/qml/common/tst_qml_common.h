// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QML_COMMON_H
#define TST_QML_COMMON_H

#include <QtCore/qstring.h>
#include <QtCore/qlibraryinfo.h>
#include <QtTest/qtest.h>
#include <QtQml/private/qqmlsignalnames_p.h>

class tst_qml_common : public QObject
{
    Q_OBJECT

private slots:
    void tst_propertyNameToChangedSignalName_data();
    void tst_propertyNameToChangedSignalName();
    void tst_propertyNameToChangedHandlerName();
    void tst_propertyNameToChangedHandlerName_data();
    void tst_signalNameToHandlerName();
    void tst_signalNameToHandlerName_data();

    void tst_changedSignalNameToPropertyName();
    void tst_changedSignalNameToPropertyName_data();
    void tst_changedHandlerNameToPropertyName();
    void tst_changedHandlerNameToPropertyName_data();
    void tst_handlerNameToSignalName();
    void tst_handlerNameToSignalName_data();

    void tst_isChangedHandlerName();
    void tst_isChangedHandlerName_data();
};

#endif // TST_QML_COMMON_H
