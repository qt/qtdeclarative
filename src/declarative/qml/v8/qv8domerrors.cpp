/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8domerrors_p.h"
#include "qv8engine_p.h"

QT_BEGIN_NAMESPACE

void qt_add_domexceptions(QV8Engine *engine)
{
    // DOM Exception
    v8::PropertyAttribute attributes = (v8::PropertyAttribute)(v8::ReadOnly | v8::DontEnum | v8::DontDelete);

    v8::Local<v8::Object> domexception = v8::Object::New();
    domexception->Set(v8::String::New("INDEX_SIZE_ERR"), v8::Integer::New(DOMEXCEPTION_INDEX_SIZE_ERR), attributes);
    domexception->Set(v8::String::New("DOMSTRING_SIZE_ERR"), v8::Integer::New(DOMEXCEPTION_DOMSTRING_SIZE_ERR), attributes);
    domexception->Set(v8::String::New("HIERARCHY_REQUEST_ERR"), v8::Integer::New(DOMEXCEPTION_HIERARCHY_REQUEST_ERR), attributes);
    domexception->Set(v8::String::New("WRONG_DOCUMENT_ERR"), v8::Integer::New(DOMEXCEPTION_WRONG_DOCUMENT_ERR), attributes);
    domexception->Set(v8::String::New("INVALID_CHARACTER_ERR"), v8::Integer::New(DOMEXCEPTION_INVALID_CHARACTER_ERR), attributes);
    domexception->Set(v8::String::New("NO_DATA_ALLOWED_ERR"), v8::Integer::New(DOMEXCEPTION_NO_DATA_ALLOWED_ERR), attributes);
    domexception->Set(v8::String::New("NO_MODIFICATION_ALLOWED_ERR"), v8::Integer::New(DOMEXCEPTION_NO_MODIFICATION_ALLOWED_ERR), attributes);
    domexception->Set(v8::String::New("NOT_FOUND_ERR"), v8::Integer::New(DOMEXCEPTION_NOT_FOUND_ERR), attributes);
    domexception->Set(v8::String::New("NOT_SUPPORTED_ERR"), v8::Integer::New(DOMEXCEPTION_NOT_SUPPORTED_ERR), attributes);
    domexception->Set(v8::String::New("INUSE_ATTRIBUTE_ERR"), v8::Integer::New(DOMEXCEPTION_INUSE_ATTRIBUTE_ERR), attributes);
    domexception->Set(v8::String::New("INVALID_STATE_ERR"), v8::Integer::New(DOMEXCEPTION_INVALID_STATE_ERR), attributes);
    domexception->Set(v8::String::New("SYNTAX_ERR"), v8::Integer::New(DOMEXCEPTION_SYNTAX_ERR), attributes);
    domexception->Set(v8::String::New("INVALID_MODIFICATION_ERR"), v8::Integer::New(DOMEXCEPTION_INVALID_MODIFICATION_ERR), attributes);
    domexception->Set(v8::String::New("NAMESPACE_ERR"), v8::Integer::New(DOMEXCEPTION_NAMESPACE_ERR), attributes);
    domexception->Set(v8::String::New("INVALID_ACCESS_ERR"), v8::Integer::New(DOMEXCEPTION_INVALID_ACCESS_ERR), attributes);
    domexception->Set(v8::String::New("VALIDATION_ERR"), v8::Integer::New(DOMEXCEPTION_VALIDATION_ERR), attributes);
    domexception->Set(v8::String::New("TYPE_MISMATCH_ERR"), v8::Integer::New(DOMEXCEPTION_TYPE_MISMATCH_ERR), attributes);
    engine->global()->Set(v8::String::New("DOMException"), domexception);
}

QT_END_NAMESPACE
