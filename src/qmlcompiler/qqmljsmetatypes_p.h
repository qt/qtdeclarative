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

#include "qqmljsannotation_p.h"

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
    QSharedPointer<const QQmlJSScope> m_type;
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

    QSharedPointer<const QQmlJSScope> type() const { return m_type; }
    void setType(const QSharedPointer<const QQmlJSScope> &type) { m_type = type; }

    friend bool operator==(const QQmlJSMetaEnum &a, const QQmlJSMetaEnum &b)
    {
        return a.m_keys == b.m_keys
                && a.m_values == b.m_values
                && a.m_name == b.m_name
                && a.m_alias == b.m_alias
                && a.m_isFlag == b.m_isFlag
                && a.m_type == b.m_type;
    }

    friend bool operator!=(const QQmlJSMetaEnum &a, const QQmlJSMetaEnum &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaEnum &e, size_t seed = 0)
    {
        return qHashMulti(seed, e.m_keys, e.m_values, e.m_name, e.m_alias, e.m_isFlag, e.m_type);
    }
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

    bool isConstructor() const { return m_isConstructor; }
    void setIsConstructor(bool isConstructor) { m_isConstructor = isConstructor; }

    bool isValid() const { return !m_name.isEmpty(); }

    const QVector<QQmlJSAnnotation>& annotations() const { return m_annotations; }
    void setAnnotations(QVector<QQmlJSAnnotation> annotations) { m_annotations = annotations; }

    friend bool operator==(const QQmlJSMetaMethod &a, const QQmlJSMetaMethod &b)
    {
        return a.m_name == b.m_name
                && a.m_returnTypeName == b.m_returnTypeName
                && a.m_returnType == b.m_returnType
                && a.m_paramNames == b.m_paramNames
                && a.m_paramTypeNames == b.m_paramTypeNames
                && a.m_paramTypes == b.m_paramTypes
                && a.m_annotations == b.m_annotations
                && a.m_methodType == b.m_methodType
                && a.m_methodAccess == b.m_methodAccess
                && a.m_revision == b.m_revision
                && a.m_isConstructor == b.m_isConstructor;
    }

    friend bool operator!=(const QQmlJSMetaMethod &a, const QQmlJSMetaMethod &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaMethod &method, size_t seed = 0)
    {
        QtPrivate::QHashCombine combine;

        seed = combine(seed, method.m_name);
        seed = combine(seed, method.m_returnTypeName);
        seed = combine(seed, method.m_returnType.toStrongRef().data());
        seed = combine(seed, method.m_paramNames);
        seed = combine(seed, method.m_paramTypeNames);
        seed = combine(seed, method.m_annotations);
        seed = combine(seed, method.m_methodType);
        seed = combine(seed, method.m_methodAccess);
        seed = combine(seed, method.m_revision);
        seed = combine(seed, method.m_isConstructor);

        for (const auto &type : method.m_paramTypes)
            seed = combine(seed, type.toStrongRef().data());

        return seed;
    }

private:
    QString m_name;
    QString m_returnTypeName;
    QWeakPointer<const QQmlJSScope> m_returnType;

    QStringList m_paramNames;
    QStringList m_paramTypeNames;
    QList<QWeakPointer<const QQmlJSScope>> m_paramTypes;
    QList<QQmlJSAnnotation> m_annotations;

    Type m_methodType = Signal;
    Access m_methodAccess = Private;
    int m_revision = 0;
    bool m_isConstructor = false;
};

class QQmlJSMetaProperty
{
    QString m_propertyName;
    QString m_typeName;
    QString m_read;
    QString m_write;
    QString m_bindable;
    QString m_notify;
    QWeakPointer<const QQmlJSScope> m_type;
    QVector<QQmlJSAnnotation> m_annotations;
    bool m_isList = false;
    bool m_isWritable = false;
    bool m_isPointer = false;
    bool m_isAlias = false;
    bool m_isFinal = false;
    int m_revision = 0;
    int m_index = -1; // relative property index within owning QQmlJSScope

public:
    QQmlJSMetaProperty() = default;

    void setPropertyName(const QString &propertyName) { m_propertyName = propertyName; }
    QString propertyName() const { return m_propertyName; }

    void setTypeName(const QString &typeName) { m_typeName = typeName; }
    QString typeName() const { return m_typeName; }

    void setRead(const QString &read) { m_read = read; }
    QString read() const { return m_read; }

    void setWrite(const QString &write) { m_write = write; }
    QString write() const { return m_write; }

    void setBindable(const QString &bindable) { m_bindable = bindable; }
    QString bindable() const { return m_bindable; }

    void setNotify(const QString &notify) { m_notify = notify; }
    QString notify() const { return m_notify; }

    void setType(const QSharedPointer<const QQmlJSScope> &type) { m_type = type; }
    QSharedPointer<const QQmlJSScope> type() const { return m_type.toStrongRef(); }

    void setAnnotations(const QList<QQmlJSAnnotation> &annotation) { m_annotations = annotation; }
    const QList<QQmlJSAnnotation> &annotations() const { return m_annotations; }

    void setIsList(bool isList) { m_isList = isList; }
    bool isList() const { return m_isList; }

    void setIsWritable(bool isWritable) { m_isWritable = isWritable; }
    bool isWritable() const { return m_isWritable; }

    void setIsPointer(bool isPointer) { m_isPointer = isPointer; }
    bool isPointer() const { return m_isPointer; }

    void setIsAlias(bool isAlias) { m_isAlias = isAlias; }
    bool isAlias() const { return m_isAlias; }

    void setIsFinal(bool isFinal) { m_isFinal = isFinal; }
    bool isFinal() const { return m_isFinal; }

    void setRevision(int revision) { m_revision = revision; }
    int revision() const { return m_revision; }

    void setIndex(int index) { m_index = index; }
    int index() const { return m_index; }

    bool isValid() const { return !m_propertyName.isEmpty(); }

    friend bool operator==(const QQmlJSMetaProperty &a, const QQmlJSMetaProperty &b)
    {
        return a.m_propertyName == b.m_propertyName && a.m_typeName == b.m_typeName
                && a.m_bindable == b.m_bindable && a.m_type == b.m_type && a.m_isList == b.m_isList
                && a.m_isWritable == b.m_isWritable && a.m_isPointer == b.m_isPointer
                && a.m_isAlias == b.m_isAlias && a.m_revision == b.m_revision
                && a.m_isFinal == b.m_isFinal;
    }

    friend bool operator!=(const QQmlJSMetaProperty &a, const QQmlJSMetaProperty &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaProperty &prop, size_t seed = 0)
    {
        return qHashMulti(seed, prop.m_propertyName, prop.m_typeName, prop.m_bindable,
                          prop.m_type.toStrongRef().data(), prop.m_isList, prop.m_isWritable,
                          prop.m_isPointer, prop.m_isAlias, prop.m_revision, prop.m_isFinal);
    }
};

class QQmlJSMetaPropertyBinding
{
    QString m_propertyName;
    QString m_valueTypeName;
    QString m_interceptorTypeName;
    QString m_valueSourceTypeName;
    QWeakPointer<const QQmlJSScope> m_value;
    QWeakPointer<const QQmlJSScope> m_interceptor;
    QWeakPointer<const QQmlJSScope> m_valueSource;

public:

    QQmlJSMetaPropertyBinding() = default;
    QQmlJSMetaPropertyBinding(const QQmlJSMetaProperty &prop) : m_propertyName(prop.propertyName())
    {
    }

    void setPropertyName(const QString &propertyName) { m_propertyName = propertyName; }
    QString propertyName() const { return m_propertyName; }

    void setValueTypeName(const QString &valueTypeName) { m_valueTypeName = valueTypeName; }
    QString valueTypeName() const { return m_valueTypeName; }

    void setInterceptorTypeName(const QString &interceptorTypeName)
    {
        m_interceptorTypeName = interceptorTypeName;
    }
    QString interceptorTypeName() const { return m_interceptorTypeName; }

    void setValueSourceTypeName(const QString &valueSourceTypeName)
    {
        m_valueSourceTypeName = valueSourceTypeName;
    }
    QString valueSourceTypeName() const { return m_valueSourceTypeName; }

    QSharedPointer<const QQmlJSScope> value() const { return m_value; }
    void setValue(const QSharedPointer<const QQmlJSScope> &value) { m_value = value; }

    QSharedPointer<const QQmlJSScope> interceptor() const { return m_interceptor; }
    void setInterceptor(const QSharedPointer<const QQmlJSScope> &interceptor)
    {
        m_interceptor = interceptor;
    }

    QSharedPointer<const QQmlJSScope> valueSource() const { return m_valueSource; }
    void setValueSource(const QSharedPointer<const QQmlJSScope> &valueSource)
    {
        m_valueSource = valueSource;
    }

    bool hasValue() const { return !m_value.isNull(); }
    bool hasInterceptor() const { return !m_interceptor.isNull(); }
    bool hasValueSource() const { return !m_valueSource.isNull(); }

    bool isValid() const { return !m_propertyName.isEmpty(); }

    friend bool operator==(const QQmlJSMetaPropertyBinding &a, const QQmlJSMetaPropertyBinding &b)
    {
        return a.m_propertyName == b.m_propertyName && a.m_valueTypeName == b.m_valueTypeName
                && a.m_interceptorTypeName == b.m_interceptorTypeName
                && a.m_valueSourceTypeName == b.m_valueSourceTypeName && a.m_value == b.m_value
                && a.m_interceptor == b.m_interceptor;
    }

    friend bool operator!=(const QQmlJSMetaPropertyBinding &a, const QQmlJSMetaPropertyBinding &b)
    {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSMetaPropertyBinding &binding, size_t seed = 0)
    {
        return qHashMulti(seed, binding.m_propertyName, binding.m_valueTypeName,
                          binding.m_interceptorTypeName, binding.m_valueSourceTypeName,
                          binding.m_value.toStrongRef().data(),
                          binding.m_interceptor.toStrongRef().data(),
                          binding.m_valueSource.toStrongRef().data());
    }
};

struct QQmlJSMetaSignalHandler
{
    QStringList signalParameters;
    bool isMultiline;
};

QT_END_NAMESPACE

#endif // QQMLJSMETATYPES_P_H
