// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
#include <QObject>
#include <QJSEngine>

class Backend : public QObject
{
    Q_OBJECT

public:
    Backend(QJSEngine *engine);

    enum Status {
        Unknown,
        Error,
        Loading,
        Loaded
    };

    Q_ENUM(Status)

    bool load();

private:
    QJSEngine *mEngine = nullptr;
};
//! [file]
