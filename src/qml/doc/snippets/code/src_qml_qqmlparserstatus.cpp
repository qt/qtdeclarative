// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qobject.h>
#include <QtQml/qqmlparserstatus.h>

//! [0]
class MyObject : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

public:
    MyObject(QObject *parent = nullptr);
    // ...
    void classBegin() override;
    void componentComplete() override;
};
//! [0]
