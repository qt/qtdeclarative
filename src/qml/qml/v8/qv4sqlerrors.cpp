/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qv4sqlerrors_p.h"
#include "private/qv4engine_p.h"
#include "private/qv4object_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

void qt_add_sqlexceptions(QV4::ExecutionEngine *engine)
{
    Scope scope(engine);
    ScopedObject sqlexception(scope, engine->newObject());
    sqlexception->defineReadonlyProperty(QStringLiteral("UNKNOWN_ERR"), Value::fromInt32(SQLEXCEPTION_UNKNOWN_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("DATABASE_ERR"), Value::fromInt32(SQLEXCEPTION_DATABASE_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("VERSION_ERR"), Value::fromInt32(SQLEXCEPTION_VERSION_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("TOO_LARGE_ERR"), Value::fromInt32(SQLEXCEPTION_TOO_LARGE_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("QUOTA_ERR"), Value::fromInt32(SQLEXCEPTION_QUOTA_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("SYNTAX_ERR"), Value::fromInt32(SQLEXCEPTION_SYNTAX_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("CONSTRAINT_ERR"), Value::fromInt32(SQLEXCEPTION_CONSTRAINT_ERR));
    sqlexception->defineReadonlyProperty(QStringLiteral("TIMEOUT_ERR"), Value::fromInt32(SQLEXCEPTION_TIMEOUT_ERR));
    engine->globalObject->defineDefaultProperty(QStringLiteral("SQLException"), sqlexception);
}

QT_END_NAMESPACE
