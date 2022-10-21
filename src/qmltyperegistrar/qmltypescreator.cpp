/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qmltypescreator.h"
#include "qmltypesclassdescription.h"

#include <QtCore/qset.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qsavefile.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qversionnumber.h>

static QString enquote(const QString &string)
{
    QString s = string;
    return QString::fromLatin1("\"%1\"").arg(s.replace(QLatin1Char('\\'), QLatin1String("\\\\"))
                                         .replace(QLatin1Char('"'),QLatin1String("\\\"")));
}

void QmlTypesCreator::writeClassProperties(const QmlTypesClassDescription &collector)
{
    if (!collector.file.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("file"), enquote(collector.file));
    m_qml.writeScriptBinding(QLatin1String("name"), enquote(collector.className));

    if (!collector.accessSemantics.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("accessSemantics"), enquote(collector.accessSemantics));

    if (!collector.defaultProp.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("defaultProperty"), enquote(collector.defaultProp));

    if (!collector.parentProp.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("parentProperty"), enquote(collector.parentProp));

    if (!collector.superClass.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("prototype"), enquote(collector.superClass));

    if (!collector.sequenceValueType.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("valueType"), enquote(collector.sequenceValueType));

    if (!collector.extensionType.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("extension"), enquote(collector.extensionType));

    if (!collector.implementsInterfaces.isEmpty()) {
        QStringList interfaces;
        for (const QString &interface : collector.implementsInterfaces)
            interfaces << enquote(interface);

        m_qml.writeArrayBinding(QLatin1String("interfaces"), interfaces);
    }

    if (collector.elementName.isEmpty())
        return;

    QStringList exports;
    QStringList metaObjects;

    for (auto it = collector.revisions.begin(), end = collector.revisions.end(); it != end; ++it) {
        const QTypeRevision revision = *it;
        if (revision < collector.addedInRevision)
            continue;
        if (collector.removedInRevision.isValid() && !(revision < collector.removedInRevision))
            break;
        if (revision.hasMajorVersion() && revision.majorVersion() > m_version.majorVersion())
            break;

        exports.append(enquote(QString::fromLatin1("%1/%2 %3.%4")
                               .arg(m_module, collector.elementName)
                               .arg(revision.hasMajorVersion() ? revision.majorVersion()
                                                               : m_version.majorVersion())
                               .arg(revision.minorVersion())));
        metaObjects.append(QString::number(revision.toEncodedVersion<quint16>()));
    }

    m_qml.writeArrayBinding(QLatin1String("exports"), exports);

    if (!collector.isCreatable || collector.isSingleton)
        m_qml.writeScriptBinding(QLatin1String("isCreatable"), QLatin1String("false"));

    if (collector.isSingleton)
        m_qml.writeScriptBinding(QLatin1String("isSingleton"), QLatin1String("true"));

    if (collector.hasCustomParser)
        m_qml.writeScriptBinding(QLatin1String("hasCustomParser"), QLatin1String("true"));

    m_qml.writeArrayBinding(QLatin1String("exportMetaObjectRevisions"), metaObjects);

    if (!collector.attachedType.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("attachedType"), enquote(collector.attachedType));
}

void QmlTypesCreator::writeType(const QJsonObject &property, const QString &key)
{
    auto it = property.find(key);
    if (it == property.end())
        return;

    QString type = (*it).toString();
    if (type.isEmpty() || type == QLatin1String("void"))
        return;

    const QLatin1String typeKey("type");

    bool isList = false;
    bool isPointer = false;

    if (type == QLatin1String("qreal")) {
#ifdef QT_COORD_TYPE_STRING
        type = QLatin1String(QT_COORD_TYPE_STRING)
#else
        type = QLatin1String("double");
#endif
    } else if (type == QLatin1String("qint32")) {
        type = QLatin1String("int");
    } else if (type == QLatin1String("quint32")) {
        type = QLatin1String("uint");
    } else if (type == QLatin1String("qint64")) {
        type = QLatin1String("qlonglong");
    } else if (type == QLatin1String("quint64")) {
        type = QLatin1String("qulonglong");
    } else {

        const QLatin1String listProperty("QQmlListProperty<");
        if (type.startsWith(listProperty)) {
            isList = true;
            const int listPropertySize = listProperty.size();
            type = type.mid(listPropertySize, type.size() - listPropertySize - 1);
        }

        if (type.endsWith(QLatin1Char('*'))) {
            isPointer = true;
            type = type.left(type.size() - 1);
        }
    }

    m_qml.writeScriptBinding(typeKey, enquote(type));
    const QLatin1String trueString("true");
    if (isList)
        m_qml.writeScriptBinding(QLatin1String("isList"), trueString);
    if (isPointer)
        m_qml.writeScriptBinding(QLatin1String("isPointer"), trueString);
}

void QmlTypesCreator::writeProperties(const QJsonArray &properties)
{
    for (const QJsonValue property : properties) {
        const QJsonObject obj = property.toObject();
        const QString name = obj[QLatin1String("name")].toString();
        m_qml.writeStartObject(QLatin1String("Property"));
        m_qml.writeScriptBinding(QLatin1String("name"), enquote(name));
        const auto it = obj.find(QLatin1String("revision"));
        if (it != obj.end())
            m_qml.writeScriptBinding(QLatin1String("revision"), QString::number(it.value().toInt()));

        writeType(obj, QLatin1String("type"));

        if (!obj.contains(QStringLiteral("privateClass"))) {
            const auto bindable = obj.constFind(QLatin1String("bindable"));
            if (bindable != obj.constEnd())
                m_qml.writeScriptBinding(QLatin1String("bindable"), enquote(bindable->toString()));
            const auto read = obj.constFind(QLatin1String("read"));
            if (read != obj.constEnd())
                 m_qml.writeScriptBinding(QLatin1String("read"), enquote(read->toString()));
            const auto write = obj.constFind(QLatin1String("write"));
            if (write != obj.constEnd())
                 m_qml.writeScriptBinding(QLatin1String("write"), enquote(write->toString()));
            const auto notify = obj.constFind(QLatin1String("notify"));
            if (notify != obj.constEnd())
                m_qml.writeScriptBinding(QLatin1String("notify"), enquote(notify->toString()));
            const auto index = obj.constFind(QLatin1String("index"));
            if (index != obj.constEnd())
                m_qml.writeScriptBinding(QLatin1String("index"),
                                         QString::number(index.value().toInt()));
        }

        if (!obj.contains(QLatin1String("write")) && !obj.contains(QLatin1String("member")))
            m_qml.writeScriptBinding(QLatin1String("isReadonly"), QLatin1String("true"));

        const auto final = obj.constFind(QLatin1String("final"));
        if (final != obj.constEnd() && final->toBool())
            m_qml.writeScriptBinding(QLatin1String("isFinal"), QLatin1String("true"));

        const auto required = obj.constFind(QLatin1String("required"));
        if (required != obj.constEnd() && required->toBool())
            m_qml.writeScriptBinding(QLatin1String("isRequired"), QLatin1String("true"));

        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeMethods(const QJsonArray &methods, const QString &type)
{
    for (const QJsonValue method : methods) {
        const QJsonObject obj = method.toObject();
        const QString name = obj[QLatin1String("name")].toString();
        if (name.isEmpty())
            continue;
        const QJsonArray arguments = method[QLatin1String("arguments")].toArray();
        const auto revision = obj.find(QLatin1String("revision"));
        m_qml.writeStartObject(type);
        m_qml.writeScriptBinding(QLatin1String("name"), enquote(name));
        if (revision != obj.end())
            m_qml.writeScriptBinding(QLatin1String("revision"), QString::number(revision.value().toInt()));
        writeType(obj, QLatin1String("returnType"));
        const auto isConstructor = obj.find(QLatin1String("isConstructor"));
        if (isConstructor != obj.constEnd() && isConstructor->toBool())
            m_qml.writeScriptBinding(QLatin1String("isConstructor"), QLatin1String("true"));
        for (const QJsonValue argument : arguments) {
            const QJsonObject obj = argument.toObject();
            m_qml.writeStartObject(QLatin1String("Parameter"));
            const QString name = obj[QLatin1String("name")].toString();
            if (!name.isEmpty())
                m_qml.writeScriptBinding(QLatin1String("name"), enquote(name));
            writeType(obj, QLatin1String("type"));
            m_qml.writeEndObject();
        }
        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeEnums(const QJsonArray &enums)
{
    for (const QJsonValue item : enums) {
        const QJsonObject obj = item.toObject();
        const QJsonArray values = obj.value(QLatin1String("values")).toArray();
        QStringList valueList;

        for (const QJsonValue value : values)
            valueList.append(enquote(value.toString()));

        m_qml.writeStartObject(QLatin1String("Enum"));
        m_qml.writeScriptBinding(QLatin1String("name"),
                               enquote(obj.value(QLatin1String("name")).toString()));
        auto alias = obj.find(QLatin1String("alias"));
        if (alias != obj.end())
            m_qml.writeScriptBinding(alias.key(), enquote(alias->toString()));
        auto isFlag = obj.find(QLatin1String("isFlag"));
        if (isFlag != obj.end() && isFlag->toBool())
            m_qml.writeBooleanBinding(isFlag.key(), true);
        m_qml.writeArrayBinding(QLatin1String("values"), valueList);
        m_qml.writeEndObject();
    }
}

static bool isAllowedInMajorVersion(const QJsonValue &member, QTypeRevision maxMajorVersion)
{
    const auto memberObject = member.toObject();
    const auto it = memberObject.find(QLatin1String("revision"));
    if (it == memberObject.end())
        return true;

    const QTypeRevision memberRevision = QTypeRevision::fromEncodedVersion(it->toInt());
    return !memberRevision.hasMajorVersion()
            || memberRevision.majorVersion() <= maxMajorVersion.majorVersion();
}

static QJsonArray members(const QJsonObject *classDef,
                          const QString &key, QTypeRevision maxMajorVersion)
{
    QJsonArray classDefMembers;

    const QJsonArray candidates = classDef->value(key).toArray();
    for (const QJsonValue member : candidates) {
        if (isAllowedInMajorVersion(member, maxMajorVersion))
            classDefMembers.append(member);
    }

    return classDefMembers;
}

void QmlTypesCreator::writeComponents()
{
    const QLatin1String nameKey("name");
    const QLatin1String signalsKey("signals");
    const QLatin1String enumsKey("enums");
    const QLatin1String propertiesKey("properties");
    const QLatin1String slotsKey("slots");
    const QLatin1String methodsKey("methods");
    const QLatin1String accessKey("access");
    const QLatin1String typeKey("type");
    const QLatin1String returnTypeKey("returnType");
    const QLatin1String argumentsKey("arguments");

    const QLatin1String destroyedName("destroyed");
    const QLatin1String deleteLaterName("deleteLater");
    const QLatin1String toStringName("toString");
    const QLatin1String destroyName("destroy");
    const QLatin1String delayName("delay");

    const QLatin1String signalElement("Signal");
    const QLatin1String componentElement("Component");
    const QLatin1String methodElement("Method");

    const QLatin1String publicAccess("public");
    const QLatin1String intType("int");
    const QLatin1String stringType("string");

    auto writeRootClass = [&](const QJsonObject *classDef) {
        // Hide destroyed() signals
        QJsonArray componentSignals = members(classDef, signalsKey, m_version);
        for (auto it = componentSignals.begin(); it != componentSignals.end();) {
            if (it->toObject().value(nameKey).toString() == destroyedName)
                it = componentSignals.erase(it);
            else
                ++it;
        }
        writeMethods(componentSignals, signalElement);

        // Hide deleteLater() methods
        QJsonArray componentMethods = members(classDef, methodsKey, m_version);
        const QJsonArray componentSlots = members(classDef, slotsKey, m_version);
        for (const QJsonValue componentSlot : componentSlots)
            componentMethods.append(componentSlot);
        for (auto it = componentMethods.begin(); it != componentMethods.end();) {
            if (it->toObject().value(nameKey).toString() == deleteLaterName)
                it = componentMethods.erase(it);
            else
                ++it;
        }

        // Add toString()
        QJsonObject toStringMethod;
        toStringMethod.insert(nameKey, toStringName);
        toStringMethod.insert(accessKey, publicAccess);
        toStringMethod.insert(returnTypeKey, stringType);
        componentMethods.append(toStringMethod);

        // Add destroy()
        QJsonObject destroyMethod;
        destroyMethod.insert(nameKey, destroyName);
        destroyMethod.insert(accessKey, publicAccess);
        componentMethods.append(destroyMethod);

        // Add destroy(int)
        QJsonObject destroyMethodWithArgument;
        destroyMethodWithArgument.insert(nameKey, destroyName);
        destroyMethodWithArgument.insert(accessKey, publicAccess);
        QJsonObject delayArgument;
        delayArgument.insert(nameKey, delayName);
        delayArgument.insert(typeKey, intType);
        QJsonArray destroyArguments;
        destroyArguments.append(delayArgument);
        destroyMethodWithArgument.insert(argumentsKey, destroyArguments);
        componentMethods.append(destroyMethodWithArgument);

        writeMethods(componentMethods, methodElement);
    };

    for (const QJsonObject &component : m_ownTypes) {
        m_qml.writeStartObject(componentElement);

        QmlTypesClassDescription collector;
        collector.collect(&component, m_ownTypes, m_foreignTypes,
                          QmlTypesClassDescription::TopLevel, m_version);

        writeClassProperties(collector);

        if (const QJsonObject *classDef = collector.resolvedClass) {
            writeEnums(members(classDef, enumsKey, m_version));

            writeProperties(members(classDef, propertiesKey, m_version));

            if (collector.isRootClass) {
                writeRootClass(classDef);
            } else {
                writeMethods(members(classDef, signalsKey, m_version), signalElement);
                writeMethods(members(classDef, slotsKey, m_version), methodElement);
                writeMethods(members(classDef, methodsKey, m_version), methodElement);
            }
        }
        m_qml.writeEndObject();

        if (collector.resolvedClass != &component
                && std::binary_search(
                    m_referencedTypes.begin(), m_referencedTypes.end(),
                    component.value(QStringLiteral("qualifiedClassName")).toString())) {

            // This type is referenced from elsewhere and has a QML_FOREIGN of its own. We need to
            // also generate a description of the local type then. All the QML_* macros are
            // ignored, and the result is an anonymous type.

            m_qml.writeStartObject(componentElement);

            QmlTypesClassDescription collector;
            collector.collectLocalAnonymous(&component, m_ownTypes, m_foreignTypes, m_version);

            writeClassProperties(collector);
            writeEnums(members(&component, enumsKey, m_version));

            writeProperties(members(&component, propertiesKey, m_version));

            writeMethods(members(&component, signalsKey, m_version), signalElement);
            writeMethods(members(&component, slotsKey, m_version), methodElement);
            writeMethods(members(&component, methodsKey, m_version), methodElement);

            m_qml.writeEndObject();
        }
    }
}

void QmlTypesCreator::generate(const QString &outFileName)
{
    m_qml.writeStartDocument();
    m_qml.writeLibraryImport(QLatin1String("QtQuick.tooling"), 1, 2);
    m_qml.write(QString::fromLatin1(
            "\n// This file describes the plugin-supplied types contained in the library."
            "\n// It is used for QML tooling purposes only."
            "\n//"
            "\n// This file was auto-generated by qmltyperegistrar.\n\n"));
    m_qml.writeStartObject(QLatin1String("Module"));

    writeComponents();

    m_qml.writeEndObject();

    QSaveFile file(outFileName);
    file.open(QIODevice::WriteOnly);
    file.write(m_output);
    file.commit();
}

