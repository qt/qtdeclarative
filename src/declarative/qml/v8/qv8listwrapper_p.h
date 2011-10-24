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

#ifndef QV8LISTWRAPPER_P_H
#define QV8LISTWRAPPER_P_H

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
#include <QtDeclarative/qdeclarativelist.h>
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QV8Engine;
class QV8ObjectResource;
class QV8ListWrapper 
{
public:
    QV8ListWrapper();
    ~QV8ListWrapper();

    void init(QV8Engine *);
    void destroy();

    v8::Handle<v8::Value> newList(QObject *, int, int);
    v8::Handle<v8::Value> newList(const QDeclarativeListProperty<QObject> &, int);
    QVariant toVariant(v8::Handle<v8::Object>);
    QVariant toVariant(QV8ObjectResource *);

private:
    static v8::Handle<v8::Value> Getter(v8::Local<v8::String> property, 
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Setter(v8::Local<v8::String> property, 
                                        v8::Local<v8::Value> value,
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> IndexedGetter(uint32_t index, 
                                               const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property, 
                                              const v8::AccessorInfo &info);
    static v8::Handle<v8::Array> Enumerator(const v8::AccessorInfo &info);

    QV8Engine *m_engine;
    v8::Persistent<v8::Function> m_constructor;
};

QT_END_NAMESPACE

#endif // QV8LISTWRAPPER_P_H

