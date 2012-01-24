/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qv8sqlerrors_p.h"
#include "qv8engine_p.h"

QT_BEGIN_NAMESPACE

void qt_add_sqlexceptions(QV8Engine *engine)
{
    // SQL Exception
    v8::PropertyAttribute attributes = (v8::PropertyAttribute)(v8::ReadOnly | v8::DontEnum | v8::DontDelete);

    v8::Local<v8::Object> sqlexception = v8::Object::New();
    sqlexception->Set(v8::String::New("UNKNOWN_ERR"), v8::Integer::New(SQLEXCEPTION_UNKNOWN_ERR), attributes);
    sqlexception->Set(v8::String::New("DATABASE_ERR"), v8::Integer::New(SQLEXCEPTION_DATABASE_ERR), attributes);
    sqlexception->Set(v8::String::New("VERSION_ERR"), v8::Integer::New(SQLEXCEPTION_VERSION_ERR), attributes);
    sqlexception->Set(v8::String::New("TOO_LARGE_ERR"), v8::Integer::New(SQLEXCEPTION_TOO_LARGE_ERR), attributes);
    sqlexception->Set(v8::String::New("QUOTA_ERR"), v8::Integer::New(SQLEXCEPTION_QUOTA_ERR), attributes);
    sqlexception->Set(v8::String::New("SYNTAX_ERR"), v8::Integer::New(SQLEXCEPTION_SYNTAX_ERR), attributes);
    sqlexception->Set(v8::String::New("CONSTRAINT_ERR"), v8::Integer::New(SQLEXCEPTION_CONSTRAINT_ERR), attributes);
    sqlexception->Set(v8::String::New("TIMEOUT_ERR"), v8::Integer::New(SQLEXCEPTION_TIMEOUT_ERR), attributes);
    engine->global()->Set(v8::String::New("SQLException"), sqlexception);
}

QT_END_NAMESPACE
