// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTTYPE_H
#define TESTTYPE_H
#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

class TypeWithVersionedAlias : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QString m_readAndWrite;

public:
    TypeWithVersionedAlias() { }
    Q_PROPERTY(QString notExisting MEMBER m_readAndWrite REVISION(6, 0));
    Q_PROPERTY(QString existing MEMBER m_readAndWrite REVISION(1, 0));
};
#endif // TESTTYPE_H
