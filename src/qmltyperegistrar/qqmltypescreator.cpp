// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltypescreator_p.h"
#include "qqmltypesclassdescription_p.h"

#include <QtCore/qset.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qsavefile.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qversionnumber.h>

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

static QString enquote(const QString &string)
{
    QString s = string;
    return QString::fromLatin1("\"%1\"").arg(s.replace(QLatin1Char('\\'), QLatin1String("\\\\"))
                                         .replace(QLatin1Char('"'),QLatin1String("\\\"")));
}

static QString convertPrivateClassToUsableForm(QString s)
{
    // typical privateClass entry in MOC looks like: ClassName::d_func(), where
    // ClassName is a non-private class name. we don't need "::d_func()" piece
    // so that could be removed, but we need "Private" so that ClassName becomes
    // ClassNamePrivate (at present, simply consider this correct)
    s.replace(u"::d_func()"_s, u"Private"_s);
    return s;
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

    if (!collector.sequenceValueType.isEmpty()) {
        const QString name = collector.sequenceValueType.endsWith('*'_L1)
                ? collector.sequenceValueType.chopped(1)
                : collector.sequenceValueType;
        m_qml.writeScriptBinding(QLatin1String("valueType"), enquote(name));
    }

    if (!collector.extensionType.isEmpty())
        m_qml.writeScriptBinding(QLatin1String("extension"), enquote(collector.extensionType));

    if (collector.extensionIsNamespace)
        m_qml.writeScriptBinding(QLatin1String("extensionIsNamespace"), QLatin1String("true"));

    if (!collector.implementsInterfaces.isEmpty()) {
        QStringList interfaces;
        for (const QString &interface : collector.implementsInterfaces)
            interfaces << enquote(interface);

        m_qml.writeArrayBinding(QLatin1String("interfaces"), interfaces);
    }

    if (!collector.deferredNames.isEmpty()) {
        QStringList deferredNames;
        for (const QString &name : collector.deferredNames)
            deferredNames << enquote(name);

        m_qml.writeArrayBinding(QLatin1String("deferredNames"), deferredNames);
    }

    if (!collector.immediateNames.isEmpty()) {
        QStringList immediateNames;
        for (const QString &name : collector.immediateNames)
            immediateNames << enquote(name);

        m_qml.writeArrayBinding(QLatin1String("immediateNames"), immediateNames);
    }

    if (collector.elementName.isEmpty()) // e.g. if QML_ANONYMOUS
        return;

    if (!collector.sequenceValueType.isEmpty()) {
        qWarning() << "Ignoring name of sequential container:" << collector.elementName;
        qWarning() << "Sequential containers are anonymous. Use QML_ANONYMOUS to register them.";
        return;
    }

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
    // This is a best effort approach (like isPointer) and will not return correct results in the
    // presence of typedefs.
    bool isConstant = false;

    auto handleList = [&](QLatin1String list) {
        if (!type.startsWith(list) || !type.endsWith(QLatin1Char('>')))
            return false;

        const int listSize = list.size();
        const QString elementType = type.mid(listSize, type.size() - listSize - 1).trimmed();

        // QQmlListProperty internally constructs the pointer. Passing an explicit '*' will
        // produce double pointers. QList is only for value types. We can't handle QLists
        // of pointers (unless specially registered, but then they're not isList).
        if (elementType.endsWith(QLatin1Char('*')))
            return false;

        isList = true;
        type = elementType;
        return true;
    };

    if (!handleList(QLatin1String("QQmlListProperty<"))
            && !handleList(QLatin1String("QList<"))) {
        if (type.endsWith(QLatin1Char('*'))) {
            isPointer = true;
            type = type.left(type.size() - 1);
        }
        if (type.startsWith(u"const ")) {
            isConstant = true;
            type = type.sliced(strlen("const "));
        }
    }

    if (type == QLatin1String("qreal")) {
#ifdef QT_COORD_TYPE_STRING
        type = QLatin1String(QT_COORD_TYPE_STRING);
#else
        type = QLatin1String("double");
#endif
    } else if (type == QLatin1String("qint16")) {
        type = QLatin1String("short");
    } else if (type == QLatin1String("quint16")) {
        type = QLatin1String("ushort");
    } else if (type == QLatin1String("qint32")) {
        type = QLatin1String("int");
    } else if (type == QLatin1String("quint32")) {
        type = QLatin1String("uint");
    } else if (type == QLatin1String("qint64")) {
        type = QLatin1String("qlonglong");
    } else if (type == QLatin1String("quint64")) {
        type = QLatin1String("qulonglong");
    } else if (type == QLatin1String("QList<QObject*>")) {
        type = QLatin1String("QObjectList");
    }

    m_qml.writeScriptBinding(typeKey, enquote(type));
    const QLatin1String trueString("true");
    if (isList)
        m_qml.writeScriptBinding(QLatin1String("isList"), trueString);
    if (isPointer)
        m_qml.writeScriptBinding(QLatin1String("isPointer"), trueString);
    if (isConstant)
        m_qml.writeScriptBinding(QLatin1String("isConstant"), trueString);
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

        const auto bindable = obj.constFind(QLatin1String("bindable"));
        if (bindable != obj.constEnd())
            m_qml.writeScriptBinding(QLatin1String("bindable"), enquote(bindable->toString()));
        const auto read = obj.constFind(QLatin1String("read"));
        if (read != obj.constEnd())
            m_qml.writeScriptBinding(QLatin1String("read"), enquote(read->toString()));
        const auto write = obj.constFind(QLatin1String("write"));
        if (write != obj.constEnd())
            m_qml.writeScriptBinding(QLatin1String("write"), enquote(write->toString()));
        const auto reset = obj.constFind(QLatin1String("reset"));
        if (reset != obj.constEnd())
            m_qml.writeScriptBinding(QLatin1String("reset"), enquote(reset->toString()));
        const auto notify = obj.constFind(QLatin1String("notify"));
        if (notify != obj.constEnd())
            m_qml.writeScriptBinding(QLatin1String("notify"), enquote(notify->toString()));
        const auto index = obj.constFind(QLatin1String("index"));
        if (index != obj.constEnd()) {
            m_qml.writeScriptBinding(QLatin1String("index"),
                                     QString::number(index.value().toInt()));
        }
        const auto privateClass = obj.constFind(QLatin1String("privateClass"));
        if (privateClass != obj.constEnd()) {
            m_qml.writeScriptBinding(
                    QLatin1String("privateClass"),
                    enquote(convertPrivateClassToUsableForm(privateClass->toString())));
        }

        if (!obj.contains(QLatin1String("write")) && !obj.contains(QLatin1String("member")))
            m_qml.writeScriptBinding(QLatin1String("isReadonly"), QLatin1String("true"));

        const auto final = obj.constFind(QLatin1String("final"));
        if (final != obj.constEnd() && final->toBool())
            m_qml.writeScriptBinding(QLatin1String("isFinal"), QLatin1String("true"));

        const auto constant = obj.constFind(QLatin1String("constant"));
        if (constant != obj.constEnd() && constant->toBool())
            m_qml.writeScriptBinding(QLatin1String("isConstant"), QLatin1String("true"));

        const auto required = obj.constFind(QLatin1String("required"));
        if (required != obj.constEnd() && required->toBool())
            m_qml.writeScriptBinding(QLatin1String("isRequired"), QLatin1String("true"));

        m_qml.writeEndObject();
    }
}

void QmlTypesCreator::writeMethods(const QJsonArray &methods, const QString &type)
{
    const auto writeFlag = [this](const QLatin1String &name, const QJsonObject &obj) {
        const auto flag = obj.find(name);
        if (flag != obj.constEnd() && flag->toBool())
            m_qml.writeBooleanBinding(name, true);
    };

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

        writeFlag(QLatin1String("isCloned"), obj);
        writeFlag(QLatin1String("isConstructor"), obj);
        writeFlag(QLatin1String("isJavaScriptFunction"), obj);

        for (qsizetype i = 0, end = arguments.size(); i != end; ++i) {
            const QJsonObject obj = arguments[i].toObject();
            if (i == 0 && end == 1 &&
                    obj[QLatin1String("type")].toString() == QLatin1String("QQmlV4Function*")) {
                m_qml.writeScriptBinding(QLatin1String("isJavaScriptFunction"),
                                         QLatin1String("true"));
                break;
            }
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
        writeType(obj, QLatin1String("type"));
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

template<typename Postprocess>
QJsonArray members(
        const QJsonObject *classDef, const QString &key, QTypeRevision maxMajorVersion,
        Postprocess &&process)
{
    QJsonArray classDefMembers;

    const QJsonArray candidates = classDef->value(key).toArray();
    for (QJsonValue member : candidates) {
        if (isAllowedInMajorVersion(member, maxMajorVersion))
            classDefMembers.append(process(std::move(member)));
    }

    return classDefMembers;
}

static QJsonArray members(
        const QJsonObject *classDef, const QString &key, QTypeRevision maxMajorVersion)
{
    return members(classDef, key, maxMajorVersion, [](QJsonValue &&member) { return member; });
}

static QJsonArray constructors(
        const QJsonObject *classDef, const QString &key, QTypeRevision maxMajorVersion)
{
    return members(classDef, key, maxMajorVersion, [](QJsonValue &&member) {
        QJsonObject ctor = member.toObject();
        ctor[QLatin1String("isConstructor")] = true;
        return ctor;
    });
}


void QmlTypesCreator::writeComponents()
{
    const QLatin1String signalsKey("signals");
    const QLatin1String enumsKey("enums");
    const QLatin1String propertiesKey("properties");
    const QLatin1String slotsKey("slots");
    const QLatin1String methodsKey("methods");
    const QLatin1String constructorsKey("constructors");

    const QLatin1String signalElement("Signal");
    const QLatin1String componentElement("Component");
    const QLatin1String methodElement("Method");

    for (const QJsonObject &component : m_ownTypes) {
        QmlTypesClassDescription collector;
        collector.collect(&component, m_ownTypes, m_foreignTypes,
                          QmlTypesClassDescription::TopLevel, m_version);

        if (collector.omitFromQmlTypes)
            continue;

        m_qml.writeStartObject(componentElement);

        writeClassProperties(collector);

        if (const QJsonObject *classDef = collector.resolvedClass) {
            writeEnums(members(classDef, enumsKey, m_version));

            writeProperties(members(classDef, propertiesKey, m_version));

            writeMethods(members(classDef, signalsKey, m_version), signalElement);
            writeMethods(members(classDef, slotsKey, m_version), methodElement);
            writeMethods(members(classDef, methodsKey, m_version), methodElement);
            writeMethods(constructors(classDef, constructorsKey, m_version), methodElement);
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
            writeMethods(constructors(&component, constructorsKey, m_version), methodElement);

            m_qml.writeEndObject();
        }
    }
}

bool QmlTypesCreator::generate(const QString &outFileName)
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
    if (!file.open(QIODevice::WriteOnly))
        return false;

    if (file.write(m_output) != m_output.size())
        return false;

    return file.commit();
}

QT_END_NAMESPACE

