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

#include "qv4domerrors_p.h"
#include "qv4object_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

void qt_add_domexceptions(ExecutionEngine *e)
{
    Scope scope(e);
    ScopedObject domexception(scope, e->newObject());
    domexception->defineReadonlyProperty(QStringLiteral("INDEX_SIZE_ERR"), Value::fromInt32(DOMEXCEPTION_INDEX_SIZE_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("DOMSTRING_SIZE_ERR"), Value::fromInt32(DOMEXCEPTION_DOMSTRING_SIZE_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("HIERARCHY_REQUEST_ERR"), Value::fromInt32(DOMEXCEPTION_HIERARCHY_REQUEST_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("WRONG_DOCUMENT_ERR"), Value::fromInt32(DOMEXCEPTION_WRONG_DOCUMENT_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("INVALID_CHARACTER_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_CHARACTER_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("NO_DATA_ALLOWED_ERR"), Value::fromInt32(DOMEXCEPTION_NO_DATA_ALLOWED_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("NO_MODIFICATION_ALLOWED_ERR"), Value::fromInt32(DOMEXCEPTION_NO_MODIFICATION_ALLOWED_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("NOT_FOUND_ERR"), Value::fromInt32(DOMEXCEPTION_NOT_FOUND_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("NOT_SUPPORTED_ERR"), Value::fromInt32(DOMEXCEPTION_NOT_SUPPORTED_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("INUSE_ATTRIBUTE_ERR"), Value::fromInt32(DOMEXCEPTION_INUSE_ATTRIBUTE_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("INVALID_STATE_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_STATE_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("SYNTAX_ERR"), Value::fromInt32(DOMEXCEPTION_SYNTAX_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("INVALID_MODIFICATION_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_MODIFICATION_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("NAMESPACE_ERR"), Value::fromInt32(DOMEXCEPTION_NAMESPACE_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("INVALID_ACCESS_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_ACCESS_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("VALIDATION_ERR"), Value::fromInt32(DOMEXCEPTION_VALIDATION_ERR));
    domexception->defineReadonlyProperty(QStringLiteral("TYPE_MISMATCH_ERR"), Value::fromInt32(DOMEXCEPTION_TYPE_MISMATCH_ERR));
    e->globalObject->defineDefaultProperty(QStringLiteral("DOMException"), domexception);
}

QT_END_NAMESPACE
