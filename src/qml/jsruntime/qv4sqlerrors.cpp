// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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
