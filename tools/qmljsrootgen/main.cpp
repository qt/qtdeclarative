/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4propertykey_p.h>
#include <QtQml/private/qv4global_p.h>
#include <QtQml/private/qv4functionobject_p.h>
#include <QtQml/qjsengine.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>

struct PropertyInfo
{
    QString name;
    QV4::PropertyAttributes attr;
    bool fromConstructor;
};

QList<PropertyInfo> getPropertyInfos(QJSValue *value, bool extractConstructor = false) {
    if (!value->isObject())
        return {};

    auto *object = QJSValuePrivate::asManagedType<QV4::Object>(value);
    auto *propertyTable = &object->internalClass()->propertyTable;

    QList<PropertyInfo> infos;
    for (int i = 0; i < propertyTable->d->alloc; i++) {
        auto &propKey = propertyTable->d->entries[i].identifier;

        if (!propKey.isValid())
            continue;

        PropertyInfo propInfo  {propKey.toQString(), object->internalClass()->propertyData.at(propertyTable->d->entries[i].index), false};
        infos << propInfo;
    }


    if (!extractConstructor || !value->hasProperty(QStringLiteral("constructor"))) {
        std::sort(infos.begin(), infos.end(), [](PropertyInfo& a, PropertyInfo& b) { return a.name < b.name; });
        return infos;
    }

    QJSValue constructor = value->property("constructor");
    auto *objectCtor = QJSValuePrivate::asManagedType<QV4::Object>(&constructor);
    auto *propertyTableCtor = &objectCtor->internalClass()->propertyTable;

    for (int i = 0; i < propertyTableCtor->d->alloc; i++) {
        auto &propKey = propertyTableCtor->d->entries[i].identifier;

        if (!propKey.isValid())
            continue;

        PropertyInfo propInfo {propKey.toQString(), objectCtor->internalClass()->propertyData.at(propertyTableCtor->d->entries[i].index), true};
        infos << propInfo;
    }


    std::sort(infos.begin(), infos.end(), [](PropertyInfo& a, PropertyInfo& b) { return a.name < b.name; });

    return infos;
}

void buildBaseType(QString className, QJsonArray *classes)
{
    QJsonObject classObject {
        {QStringLiteral("className"), className},
        {QStringLiteral("qualifiedClassName"), className},
        {QStringLiteral("object"), true},
        {QStringLiteral("classInfos"),
                    QJsonArray({
                                   QJsonObject({
                                       { QStringLiteral("name"), QStringLiteral("QML.Element") },
                                       { QStringLiteral("value"), QStringLiteral("anonymous") }
                                   })
                               })
        }
    };

    classes->append(classObject);
}

enum class SeenType {
    Normal,
    Constructed
};

void buildClass(QJSValue value, QJsonArray *classes, bool globalObject, QMap<QString, SeenType> &seen, SeenType seenType = SeenType::Normal) {
    QJsonObject classObject;

    auto *object = QJSValuePrivate::asManagedType<QV4::Object>(&value);

    QString className = globalObject ? QStringLiteral("GlobalObject") : QString::fromUtf8(object->vtable()->className);

    // Make sure we're not building the same class twice if it has been fully seen
    if (seen.contains(className) && (seen[className] != SeenType::Constructed || seenType == SeenType::Constructed))
        return;

    seen.insert(className, seenType);

    // See if there is a lesser duplicate that needs to be removed.
    for (auto it = classes->begin(); it != classes->end(); ++it) {
        if (it->toObject()[QStringLiteral("className")] == className) {
           it = classes->erase(it);
           break;
        }
    }

    classObject[QStringLiteral("className")] = className;
    classObject[QStringLiteral("qualifiedClassName")] = className;

    classObject[QStringLiteral("classInfos")] = QJsonArray({
                                                          QJsonObject({
                                                              { QStringLiteral("name"), QStringLiteral("QML.Element") },
                                                              { QStringLiteral("value"), globalObject ? QStringLiteral("auto") : QStringLiteral("anonymous") }
                                                          })
                                                      });

    classObject[QStringLiteral("object")] = true;

    QV4::Scope scope(object);

    QJSValue prototype = value.prototype();

    // Try to see whether calling the prototype constructor or the prototype's prototype constructor uncovers any types.
    // (It usually doesn't)
    if (prototype.property("constructor").isCallable()) {
        buildClass(prototype.property("constructor").callAsConstructor(), classes, false, seen, SeenType::Constructed);
    }

    if (prototype.prototype().property("constructor").isCallable()) {
        buildClass(prototype.prototype().property("constructor").callAsConstructor(), classes, false, seen, SeenType::Constructed);
    }

    classObject[QStringLiteral("superClasses")] = QJsonArray {
            QJsonObject ({
                             { QStringLiteral("access"), QStringLiteral("public") },
                             { QStringLiteral("name"), className == QStringLiteral("Object") ? QStringLiteral("QJSValue") : QStringLiteral("Object") }
                         })};

    QJsonArray properties, methods;

    for (const PropertyInfo &info : getPropertyInfos(&value, className == QStringLiteral("Object"))) {
        QJSValue prop;

        if (info.fromConstructor)
            prop = value.property("constructor").property(info.name);
        else
            prop = value.property(info.name);

        // This appears in many property tables despite not being real properties
        if (info.name.startsWith(QStringLiteral("@Symbol.")))
            continue;


        // Method or constructor
        if (prop.isCallable()) {
            const QV4::FunctionObject *propFunction = QJSValuePrivate::asManagedType<QV4::FunctionObject>(&prop);

            QJsonObject methodObject;

            methodObject.insert(QStringLiteral("access"), QStringLiteral("public"));
            methodObject.insert(QStringLiteral("name"), info.name);

            if (propFunction->isConstructor()) {
                methodObject.insert(QStringLiteral("isConstructor"), true);


                QJSValue constructed = prop.callAsConstructor();
                if (constructed.isObject()) {
                    auto *constructedObject = QJSValuePrivate::asManagedType<QV4::Object>(&constructed);
                    QString classObjectType = constructedObject->vtable()->className;

                    buildClass(constructed, classes, false, seen, SeenType::Constructed);

                    methodObject.insert(QStringLiteral("returnType"), classObjectType);
                } else {
                    qWarning() << "Warning: Calling constructor" << info.name << "failed";
                }
            }

            const int formalParams = propFunction->getLength();

            QJsonArray arguments;
            for (int i = 0; i < formalParams; i++) {
                arguments.append(QJsonObject {});
            }

            methodObject.insert(QStringLiteral("arguments"), arguments);

            methods.append(methodObject);

            continue;
        }

        // ...else it's just a property
        QJsonObject propertyObject;

        propertyObject.insert(QStringLiteral("name"), info.name);

        // Insert faux member entry if we're allowed to write to this
        if (info.attr.isWritable())
            propertyObject.insert(QStringLiteral("member"), QStringLiteral("fakeMember"));

        // Writing out the types is kind of hard in this case because we often have no corresponding QObject type
        if (prop.isQObject()) {
            propertyObject.insert(QStringLiteral("type"), prop.toQObject()->metaObject()->className());
        } else {
            QString type;

            if (prop.isString()) {
                type = QStringLiteral("string");
            } else if (prop.isBool()){
                type = QStringLiteral("bool");
            } else if (prop.isNumber()) {
                type = QStringLiteral("number");
            } else if (prop.isUndefined()) {
                type = QStringLiteral("undefined");
            } else if (prop.isArray() || prop.isNull() || prop.isObject()) {
                type = QStringLiteral("object");
            } else {
                qWarning() << "Warning: Failed to resolve type of property" << info.name;
                type = QStringLiteral("undefined");
            }

            if (seenType != SeenType::Constructed || !prop.isUndefined())
                propertyObject.insert(QStringLiteral("type"), type);
        }

        if (prop.isObject() && !prop.isQObject()) {
            buildClass(prop, classes, false, seen);

            auto *object = QJSValuePrivate::asManagedType<QV4::Object>(&prop);

            propertyObject.insert(QStringLiteral("type"), object->vtable()->className);
        }

        properties.append(propertyObject);
    }

    classObject[QStringLiteral("properties")] = properties;
    classObject[QStringLiteral("methods")] = methods;

    // Make sure that in the unlikely accident that some subclass has already provided a normal replacement for this constructed type
    // there are no duplicate entries.
    if (seenType != SeenType::Constructed || seen[className] != SeenType::Normal)
        classes->append(classObject);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QStringList args = app.arguments();

    if (args.length() != 2) {
        qWarning().noquote() << app.applicationName() << "[output json path]";
        return 1;
    }

    QString fileName = args.at(1);

    QJSEngine engine;
    engine.installExtensions(QJSEngine::AllExtensions);

    QJsonArray classesArray;

    // Add JS types so they can be referenced by other classes
    for (const QString &name : { QStringLiteral("string"), QStringLiteral("undefined"), QStringLiteral("number"),
                                 QStringLiteral("object"), QStringLiteral("bool"), QStringLiteral("symbol"), QStringLiteral("function")}) {
        buildBaseType(name, &classesArray);
    }

    QMap<QString, SeenType> seen {};

    buildClass(engine.globalObject(), &classesArray, true, seen);

    // Generate the fake metatypes json structure
    QJsonDocument metatypesJson = QJsonDocument(
                QJsonArray({
                               QJsonObject({
                                   {QStringLiteral("classes"), classesArray}
                               })
                           })
                );

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        qWarning() << "Failed to write metatypes json to" << fileName;
        return 1;
    }

    file.write(metatypesJson.toJson());
    file.close();

    return 0;
}
