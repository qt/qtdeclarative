/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV8DOMERRORS_P_H
#define QV8DOMERRORS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
// From DOM-Level-3-Core spec
// http://www.w3.org/TR/DOM-Level-3-Core/core.html
#define DOMEXCEPTION_INDEX_SIZE_ERR 1
#define DOMEXCEPTION_DOMSTRING_SIZE_ERR 2
#define DOMEXCEPTION_HIERARCHY_REQUEST_ERR 3
#define DOMEXCEPTION_WRONG_DOCUMENT_ERR 4
#define DOMEXCEPTION_INVALID_CHARACTER_ERR 5
#define DOMEXCEPTION_NO_DATA_ALLOWED_ERR 6
#define DOMEXCEPTION_NO_MODIFICATION_ALLOWED_ERR 7
#define DOMEXCEPTION_NOT_FOUND_ERR 8
#define DOMEXCEPTION_NOT_SUPPORTED_ERR 9
#define DOMEXCEPTION_INUSE_ATTRIBUTE_ERR 10
#define DOMEXCEPTION_INVALID_STATE_ERR 11
#define DOMEXCEPTION_SYNTAX_ERR 12
#define DOMEXCEPTION_INVALID_MODIFICATION_ERR 13
#define DOMEXCEPTION_NAMESPACE_ERR 14
#define DOMEXCEPTION_INVALID_ACCESS_ERR 15
#define DOMEXCEPTION_VALIDATION_ERR 16
#define DOMEXCEPTION_TYPE_MISMATCH_ERR 17

#define V8THROW_DOM(error, string) { \
    v8::Local<v8::Value> v = v8::Exception::Error(v8::String::New(string)); \
    v->ToObject()->Set(v8::String::New("code"), v8::Integer::New(error)); \
    v8::ThrowException(v); \
    return v8::Handle<v8::Value>(); \
}
class QV8Engine;
void qt_add_domexceptions(QV8Engine *engine);

QT_END_NAMESPACE

#endif // QV8DOMERRORS_P_H
