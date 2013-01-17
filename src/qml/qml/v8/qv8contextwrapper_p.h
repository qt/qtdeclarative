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

#ifndef QV8CONTEXTWRAPPER_P_H
#define QV8CONTEXTWRAPPER_P_H

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
#include <private/qtqmlglobal_p.h>
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QUrl;
class QObject;
class QV8Engine;
class QQmlContextData;
class Q_QML_PRIVATE_EXPORT QV8ContextWrapper
{
public:
    QV8ContextWrapper();
    ~QV8ContextWrapper();

    void init(QV8Engine *);
    void destroy();

    v8::Local<v8::Object> qmlScope(QQmlContextData *ctxt, QObject *scope);
    v8::Local<v8::Object> urlScope(const QUrl &);

    void setReadOnly(v8::Handle<v8::Object>, bool);

    void addSubContext(v8::Handle<v8::Object> qmlglobal, v8::Handle<v8::Script>, 
                       QQmlContextData *ctxt);

    QQmlContextData *callingContext();
    QQmlContextData *context(v8::Handle<v8::Value>);

    inline v8::Handle<v8::Object> sharedContext() const;

    void takeContextOwnership(v8::Handle<v8::Object> qmlglobal);

private:
    static v8::Handle<v8::Value> NullGetter(v8::Local<v8::String> property, 
                                            const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> NullSetter(v8::Local<v8::String> property, 
                                            v8::Local<v8::Value> value,
                                            const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Getter(v8::Local<v8::String> property, 
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Setter(v8::Local<v8::String> property, 
                                        v8::Local<v8::Value> value,
                                        const v8::AccessorInfo &info);

    QV8Engine *m_engine;
    v8::Persistent<v8::Function> m_constructor;
    v8::Persistent<v8::Function> m_urlConstructor;
    v8::Persistent<v8::Object> m_sharedContext;
};

v8::Handle<v8::Object> QV8ContextWrapper::sharedContext() const
{
    return m_sharedContext;
}

QT_END_NAMESPACE

#endif // QV8CONTEXTWRAPPER_P_H

