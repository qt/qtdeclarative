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

#ifndef QQMLJSMETATYPES_P_H
#define QQMLJSMETATYPES_P_H

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
#include <QtCore/qsharedpointer.h>

// MetaMethod and MetaProperty have both type names and actual QQmlJSScope types.
// When parsing the information from the relevant QML or qmltypes files, we only
// see the names and don't have a complete picture of the types, yet. In a second
// pass we typically fill in the types. The types may have multiple exported names
// and the the name property of MetaProperty and MetaMethod still carries some
// significance regarding which name was chosen to refer to the type. In a third
// pass we may further specify the type if the context provides additional information.
// The parent of an Item, for example, is typically not just a QtObject, but rather
// some other Item with custom properties.

QT_BEGIN_NAMESPACE

class QQmlJSScope;
class QQmlJSMetaEnum
{
    QStringList m_keys;
    QList<int> m_values; // empty if values unknown.
    QString m_name;
    QString m_alias;
    bool m_isFlag = false;

public:
    QQmlJSMetaEnum() = default;
    explicit QQmlJSMetaEnum(QString name) : m_name(std::move(name)) {}

    bool isValid() const { return !m_name.isEmpty(); }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString alias() const { return m_alias; }
    void setAlias(const QString &alias) { m_alias = alias; }

    bool isFlag() const { return m_isFlag; }
    void setIsFlag(bool isFlag) { m_isFlag = isFlag; }

    void addKey(const QString &key) { m_keys.append(key); }
    QStringList keys() const { return m_keys; }

    void addValue(int value) { m_values.append(value); }
    QList<int> values() const { return m_values; }

    bool hasValues() const { return !m_values.isEmpty(); }
    int value(const QString &key) const { return m_values.value(m_keys.indexOf(key)); }
    bool hasKey(const QString &key) const { return m_keys.indexOf(key) != -1; }
};

class QQmlJSMetaMethod
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

    QQmlJSMetaMethod() = default;
    explicit QQmlJSMetaMethod(QString name, QString returnType = QString())
        : m_name(std::move(name))
        , m_returnTypeName(std::move(returnType))
        , m_methodType(Method)
        , m_methodAccess(Public)
    {}

    QString methodName() const { return m_name; }
    void setMethodName(const QString &name) { m_name = name; }

    QString returnTypeName() const { return m_returnTypeName; }
    QSharedPointer<const QQmlJSScope> returnType() const { return m_returnType.toStrongRef(); }
    void setReturnTypeName(const QString &type) { m_returnTypeName = type; }
    void setReturnType(const QSharedPointer<const QQmlJSScope> &type)
    {
        m_returnType = type;
    }

    QStringList parameterNames() const { return m_paramNames; }
    QStringList parameterTypeNames() const { return m_paramTypeNames; }
    QList<QSharedPointer<const QQmlJSScope>> parameterTypes() const
    {
        QList<QSharedPointer<const QQmlJSScope>> result;
        for (const auto &type : m_paramTypes)
            result.append(type.toStrongRef());
        return result;
    }
    void setParameterTypes(const QList<QSharedPointer<const QQmlJSScope>> &types)
    {
        Q_ASSERT(types.length() == m_paramNames.length());
        m_paramTypes.clear();
        for (const auto &type : types)
            m_paramTypes.append(type);
    }
    void addParameter(const QString &name, const QString &typeName,
                      const QSharedPointer<const QQmlJSScope> &type = {})
    {
        m_paramNames.append(name);
        m_paramTypeNames.append(typeName);
        m_paramTypes.append(type);
    }

    int methodType() const { return m_methodType; }
    void setMethodType(Type methodType) { m_methodType = methodType; }

    Access access() const { return m_methodAccess; }

    int revision() const { return m_revision; }
    void setRevision(int r) { m_revision = r; }

    bool isValid() const { return !m_name.isEmpty(); }

private:
    QString m_name;
    QString m_returnTypeName;
    QWeakPointer<const QQmlJSScope> m_returnType;

    QStringList m_paramNames;
    QStringList m_paramTypeNames;
    QList<QWeakPointer<const QQmlJSScope>> m_paramTypes;

    Type m_methodType = Signal;
    Access m_methodAccess = Private;
    int m_revision = 0;
};

class QQmlJSMetaProperty
{
    QString m_propertyName;
    QString m_typeName;
    QString m_bindable;
    QWeakPointer<const QQmlJSScope> m_type;
    bool m_isList = false;
    bool m_isWritable = false;
    bool m_isPointer = false;
    bool m_isAlias = false;
    int m_revision = 0;

public:
    QQmlJSMetaProperty() = default;

    void setPropertyName(const QString &propertyName) { m_propertyName = propertyName; }
    QString propertyName() const { return m_propertyName; }

    void setTypeName(const QString &typeName) { m_typeName = typeName; }
    QString typeName() const { return m_typeName; }

    void setBindable(const QString &bindable) { m_bindable = bindable; }
    QString bindable() const { return m_bindable; }

    void setType(const QSharedPointer<const QQmlJSScope> &type) { m_type = type; }
    QSharedPointer<const QQmlJSScope> type() const { return m_type.toStrongRef(); }

    void setIsList(bool isList) { m_isList = isList; }
    bool isList() const { return m_isList; }

    void setIsWritable(bool isWritable) { m_isWritable = isWritable; }
    bool isWritable() const { return m_isWritable; }

    void setIsPointer(bool isPointer) { m_isPointer = isPointer; }
    bool isPointer() const { return m_isPointer; }

    void setIsAlias(bool isAlias) { m_isAlias = isAlias; }
    bool isAlias() const { return m_isAlias; }

    void setRevision(int revision) { m_revision = revision; }
    int revision() const { return m_revision; }
};

QT_END_NAMESPACE

#endif // QQMLJSMETATYPES_P_H
