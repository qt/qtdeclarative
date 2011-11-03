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

#ifndef QDECLARATIVEBUILTINFUNCTIONS_P_H
#define QDECLARATIVEBUILTINFUNCTIONS_P_H

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
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

namespace QDeclarativeBuiltinFunctions
{
v8::Handle<v8::Value> gc(const v8::Arguments &args);
v8::Handle<v8::Value> print(const v8::Arguments &args);
v8::Handle<v8::Value> isQtObject(const v8::Arguments &args);
v8::Handle<v8::Value> rgba(const v8::Arguments &args);
v8::Handle<v8::Value> hsla(const v8::Arguments &args);
v8::Handle<v8::Value> rect(const v8::Arguments &args);
v8::Handle<v8::Value> point(const v8::Arguments &args);
v8::Handle<v8::Value> size(const v8::Arguments &args);
v8::Handle<v8::Value> vector3d(const v8::Arguments &args);
v8::Handle<v8::Value> vector4d(const v8::Arguments &args);
v8::Handle<v8::Value> lighter(const v8::Arguments &args);
v8::Handle<v8::Value> darker(const v8::Arguments &args);
v8::Handle<v8::Value> tint(const v8::Arguments &args);
v8::Handle<v8::Value> formatDate(const v8::Arguments &args);
v8::Handle<v8::Value> formatTime(const v8::Arguments &args);
v8::Handle<v8::Value> formatDateTime(const v8::Arguments &args);
v8::Handle<v8::Value> openUrlExternally(const v8::Arguments &args);
v8::Handle<v8::Value> fontFamilies(const v8::Arguments &args);
v8::Handle<v8::Value> md5(const v8::Arguments &args);
v8::Handle<v8::Value> btoa(const v8::Arguments &args);
v8::Handle<v8::Value> atob(const v8::Arguments &args);
v8::Handle<v8::Value> quit(const v8::Arguments &args);
v8::Handle<v8::Value> resolvedUrl(const v8::Arguments &args);
v8::Handle<v8::Value> createQmlObject(const v8::Arguments &args);
v8::Handle<v8::Value> createComponent(const v8::Arguments &args);
v8::Handle<v8::Value> qsTranslate(const v8::Arguments &args);
v8::Handle<v8::Value> qsTranslateNoOp(const v8::Arguments &args);
v8::Handle<v8::Value> qsTr(const v8::Arguments &args);
v8::Handle<v8::Value> qsTrNoOp(const v8::Arguments &args);
v8::Handle<v8::Value> qsTrId(const v8::Arguments &args);
v8::Handle<v8::Value> qsTrIdNoOp(const v8::Arguments &args);
v8::Handle<v8::Value> consoleTime(const v8::Arguments &args);
v8::Handle<v8::Value> consoleTimeEnd(const v8::Arguments &args);
v8::Handle<v8::Value> stringArg(const v8::Arguments &args);
}

QT_END_NAMESPACE

#endif // QDECLARATIVEBUILTINFUNCTIONS_P_H
