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

#include "qv8stringwrapper_p.h"
#include "qjsconverter_p.h"
#include "qjsconverter_impl_p.h"

QT_BEGIN_NAMESPACE

QV8StringWrapper::QV8StringWrapper()
{
}

QV8StringWrapper::~QV8StringWrapper()
{
}

void QV8StringWrapper::init()
{
}

void QV8StringWrapper::destroy()
{
}

v8::Local<v8::String> QV8StringWrapper::toString(const QString &qstr)
{
    return QJSConverter::toString(qstr);
}

QString QV8StringWrapper::toString(v8::Handle<v8::String> jsstr)
{
    if (jsstr.IsEmpty()) {
        return QString();
    } else {
        return QJSConverter::toString(jsstr);
    }
}

QT_END_NAMESPACE
