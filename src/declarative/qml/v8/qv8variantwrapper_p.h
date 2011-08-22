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

#ifndef QV8VARIANTWRAPPER_P_H
#define QV8VARIANTWRAPPER_P_H

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
class QV8VariantWrapper 
{
public:
    QV8VariantWrapper();
    ~QV8VariantWrapper();

    void init(QV8Engine *);
    void destroy();

    v8::Local<v8::Object> newVariant(const QVariant &);
    bool isVariant(v8::Handle<v8::Value>);
    QVariant toVariant(v8::Handle<v8::Object>);
    QVariant toVariant(QV8ObjectResource *);
    QVariant &variantValue(v8::Handle<v8::Value>);

private:
    static v8::Handle<v8::Value> Getter(v8::Local<v8::String> property, 
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Setter(v8::Local<v8::String> property, 
                                        v8::Local<v8::Value> value,
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> PreserveGetter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> DestroyGetter(v8::Local<v8::String> property, 
                                               const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> ToStringGetter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> ValueOfGetter(v8::Local<v8::String> property,
                                               const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Preserve(const v8::Arguments &args);
    static v8::Handle<v8::Value> Destroy(const v8::Arguments &args);
    static v8::Handle<v8::Value> ToString(const v8::Arguments &args);
    static v8::Handle<v8::Value> ValueOf(const v8::Arguments &args);

    QV8Engine *m_engine;
    v8::Persistent<v8::Function> m_constructor;
    v8::Persistent<v8::Function> m_scarceConstructor;
    v8::Persistent<v8::Function> m_preserve;
    v8::Persistent<v8::Function> m_destroy;
    v8::Persistent<v8::Function> m_toString;
    v8::Persistent<v8::Function> m_valueOf;
};

QT_END_NAMESPACE

#endif // QV8VARIANTWRAPPER_P_H

