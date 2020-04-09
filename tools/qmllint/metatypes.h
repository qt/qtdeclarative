/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef METATYPES_H
#define METATYPES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

class MetaEnum
{
    QStringList m_keys;
    QString m_name;
    QString m_alias;
    bool m_isFlag = false;

public:
    MetaEnum() = default;
    explicit MetaEnum(QString name) : m_name(std::move(name)) {}

    bool isValid() const { return !m_name.isEmpty(); }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString alias() const { return m_alias; }
    void setAlias(const QString &alias) { m_alias = alias; }

    bool isFlag() const { return m_isFlag; }
    void setIsFlag(bool isFlag) { m_isFlag = isFlag; }

    void addKey(const QString &key) { m_keys.append(key); }
    QStringList keys() const { return m_keys; }
};

class MetaMethod
{
public:
    enum Type {
        Signal,
        Slot,
        Method
    };

    enum Access {
        Private,
        Protected,
        Public
    };

    MetaMethod() = default;
    explicit MetaMethod(QString name, QString returnType = QString())
        : m_name(std::move(name))
        , m_returnType(std::move(returnType))
        , m_methodType(Method)
        , m_methodAccess(Public)
    {}

    QString methodName() const { return m_name; }
    void setMethodName(const QString &name) { m_name = name; }

    void setReturnType(const QString &type) { m_returnType = type; }

    QStringList parameterNames() const { return m_paramNames; }
    QStringList parameterTypes() const { return m_paramTypes; }
    void addParameter(const QString &name, const QString &type)
    {
        m_paramNames.append(name);
        m_paramTypes.append(type);
    }

    int methodType() const { return m_methodType; }
    void setMethodType(Type methodType) { m_methodType = methodType; }

    Access access() const { return m_methodAccess; }

    int revision() const { return m_revision; }
    void setRevision(int r) { m_revision = r; }

private:
    QString m_name;
    QString m_returnType;
    QStringList m_paramNames;
    QStringList m_paramTypes;
    Type m_methodType = Signal;
    Access m_methodAccess = Private;
    int m_revision = 0;
};

class ScopeTree;
class MetaProperty
{
    QString m_propertyName;
    QString m_typeName;
    const ScopeTree *m_type = nullptr;
    bool m_isList;
    bool m_isWritable;
    bool m_isPointer;
    bool m_isAlias;
    int m_revision;

public:
    MetaProperty(QString propertyName, QString typeName,
                 bool isList, bool isWritable, bool isPointer, bool isAlias,
                 int revision)
        : m_propertyName(std::move(propertyName))
        , m_typeName(std::move(typeName))
        , m_isList(isList)
        , m_isWritable(isWritable)
        , m_isPointer(isPointer)
        , m_isAlias(isAlias)
        , m_revision(revision)
    {}

    QString propertyName() const { return m_propertyName; }
    QString typeName() const { return m_typeName; }

    void setType(const ScopeTree *type) { m_type = type; }
    const ScopeTree *type() const { return m_type; }

    bool isList() const { return m_isList; }
    bool isWritable() const { return m_isWritable; }
    bool isPointer() const { return m_isPointer; }
    bool isAlias() const { return m_isAlias; }
    int revision() const { return m_revision; }
};

#endif // METATYPES_H
