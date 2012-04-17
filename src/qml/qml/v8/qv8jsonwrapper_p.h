/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV8JSONWRAPPER_P_H
#define QV8JSONWRAPPER_P_H

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
#include <QtCore/qset.h>
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QJsonValue;
class QJsonObject;
class QJsonArray;

class QV8Engine;
class QV8JsonWrapper
{
public:
    QV8JsonWrapper();
    ~QV8JsonWrapper();

    void init(QV8Engine *);
    void destroy();

    v8::Handle<v8::Value> fromJsonValue(const QJsonValue &value);
    QJsonValue toJsonValue(v8::Handle<v8::Value> value);

    v8::Local<v8::Object> fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject(v8::Handle<v8::Value> value);

    v8::Local<v8::Array> fromJsonArray(const QJsonArray &array);
    QJsonArray toJsonArray(v8::Handle<v8::Value> value);

private:
    QV8Engine *m_engine;
    QSet<int> m_visitedConversionObjects;
};

QT_END_NAMESPACE

#endif // QV8JSONWRAPPER_P_H

