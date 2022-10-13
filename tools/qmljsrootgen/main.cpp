// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4propertykey_p.h>
#include <QtQml/private/qv4global_p.h>
#include <QtQml/private/qv4functionobject_p.h>
#include <QtQml/qjsengine.h>
#include <QtQml/qjsmanagedvalue.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>

struct PropertyInfo
{
    QString name;
    bool writable;
};

static QV4::ReturnedValue asManaged(const QJSManagedValue &value)
{
    const QJSValue jsVal = value.toJSValue();
    const QV4::Managed *managed = QJSValuePrivate::asManagedType<QV4::Managed>(&jsVal);
    return managed ? managed->asReturnedValue() : QV4::Encode::undefined();
}

static QJSManagedValue checkedProperty(const QJSManagedValue &value, const QString &name)
{
    return value.hasProperty(name) ? QJSManagedValue(value.property(name), value.engine())
                                   : QJSManagedValue(QJSPrimitiveUndefined(), value.engine());
}

QList<PropertyInfo> getPropertyInfos(const QJSManagedValue &value)
{
    QV4::Scope scope(value.engine()->handle());
    QV4::ScopedObject scoped(scope, asManaged(value));
    if (!scoped)
        return {};

    QList<PropertyInfo> infos;

    QScopedPointer<QV4::OwnPropertyKeyIterator> iterator(scoped->ownPropertyKeys(scoped));
    QV4::Scoped<QV4::InternalClass> internalClass(scope, scoped->internalClass());

    for (auto key = iterator->next(scoped); key.isValid(); key = iterator->next(scoped)) {
        if (key.isSymbol())
            continue;

        const auto *entry = internalClass->d()->propertyTable.lookup(key);
        infos.append({
            key.toQString(),
            !entry || internalClass->d()->propertyData.at(entry->index).isWritable()
        });
    };

    return infos;
}

struct State {
    QMap<QString, QJSValue> constructors;
    QMap<QString, QJSValue> prototypes;
    QSet<QString> primitives;
};

static QString buildConstructor(const QJSManagedValue &constructor, QJsonArray *classes,
                                State *seen, const QString &name, QJSManagedValue *constructed);

static QString findClassName(const QJSManagedValue &value)
{
    if (value.isUndefined())
        return QStringLiteral("undefined");
    if (value.isBoolean())
        return QStringLiteral("boolean");
    if (value.isNumber())
        return QStringLiteral("number");
    if (value.isString())
        return QStringLiteral("string");
    if (value.isSymbol())
        return QStringLiteral("symbol");

    QV4::Scope scope(value.engine()->handle());
    if (QV4::ScopedValue scoped(scope, asManaged(value)); scoped->isManaged())
        return scoped->managed()->vtable()->className;

    Q_UNREACHABLE_RETURN(QString());
}

static QString buildClass(const QJSManagedValue &value, QJsonArray *classes,
                          State *seen, const QString &name)
{
    if (value.isNull())
        return QString();

    if (seen->primitives.contains(name))
        return name;
    else if (name.at(0).isLower())
        seen->primitives.insert(name);

    QJsonObject classObject;
    QV4::Scope scope(value.engine()->handle());

    classObject[QStringLiteral("className")] = name;
    classObject[QStringLiteral("qualifiedClassName")] = name;

    classObject[QStringLiteral("classInfos")] = QJsonArray({
                                                          QJsonObject({
                                                              { QStringLiteral("name"), QStringLiteral("QML.Element") },
                                                              { QStringLiteral("value"), QStringLiteral("anonymous") }
                                                          })
                                                      });

    if (value.isObject() || value.isFunction())
        classObject[QStringLiteral("object")] = true;
    else
        classObject[QStringLiteral("gadget")] = true;

    const QJSManagedValue prototype = value.prototype();

    if (!prototype.isNull()) {
        QString protoName;
        for (auto it = seen->prototypes.begin(), end = seen->prototypes.end(); it != end; ++it) {
            if (prototype.strictlyEquals(QJSManagedValue(*it, value.engine()))) {
                protoName = it.key();
                break;
            }
        }

        if (protoName.isEmpty()) {
            if (name.endsWith(QStringLiteral("ErrorPrototype"))
                    && name != QStringLiteral("ErrorPrototype")) {
                protoName = QStringLiteral("ErrorPrototype");
            } else if (name.endsWith(QStringLiteral("Prototype"))) {
                protoName = findClassName(prototype);
                if (!protoName.endsWith(QStringLiteral("Prototype")))
                    protoName += QStringLiteral("Prototype");
            } else {
                protoName = name.at(0).toUpper() + name.mid(1) + QStringLiteral("Prototype");
            }

            auto it = seen->prototypes.find(protoName);
            if (it == seen->prototypes.end()) {
                seen->prototypes.insert(protoName, prototype.toJSValue());
                buildClass(prototype, classes, seen, protoName);
            } else if (!it->strictlyEquals(prototype.toJSValue())) {
                qWarning() << "Cannot find a distinct name for the prototype of" << name;
                qWarning() << protoName << "is already in use.";
            }
        }

        classObject[QStringLiteral("superClasses")] = QJsonArray {
                QJsonObject ({
                                 { QStringLiteral("access"), QStringLiteral("public") },
                                 { QStringLiteral("name"), protoName }
                             })};
    }

    QJsonArray properties, methods;

    auto defineProperty = [&](const QJSManagedValue &prop, const PropertyInfo &info) {
        QJsonObject propertyObject;
        propertyObject.insert(QStringLiteral("name"), info.name);

        // Insert faux member entry if we're allowed to write to this
        if (info.writable)
            propertyObject.insert(QStringLiteral("member"), QStringLiteral("fakeMember"));

        if (!prop.isUndefined() && !prop.isNull()) {
            QString propClassName = findClassName(prop);
            if (!propClassName.at(0).isLower() && info.name != QStringLiteral("prototype")) {
                propClassName = (name == QStringLiteral("GlobalObject"))
                        ? QString()
                        : name.at(0).toUpper() + name.mid(1);

                propClassName += info.name.at(0).toUpper() + info.name.mid(1);
                propertyObject.insert(QStringLiteral("type"),
                                      buildClass(prop, classes, seen, propClassName));
            } else {
                // If it's the "prototype" property we just refer to generic "Object",
                // and if it's a value type, we handle it separately.
                propertyObject.insert(QStringLiteral("type"), propClassName);
            }
        }
        return propertyObject;
    };

    QList<PropertyInfo> unRetrievedProperties;
    QJSManagedValue constructed;
    for (const PropertyInfo &info : getPropertyInfos(value)) {
        QJSManagedValue prop = checkedProperty(value, info.name);
        if (prop.engine()->hasError()) {
            unRetrievedProperties.append(info);
            prop.engine()->catchError();
            continue;
        }

        // Method or constructor
        if (prop.isFunction()) {
            QV4::Scoped<QV4::FunctionObject> propFunction(scope, asManaged(prop));

            QJsonObject methodObject;

            methodObject.insert(QStringLiteral("access"), QStringLiteral("public"));
            methodObject.insert(QStringLiteral("name"), info.name);
            methodObject.insert(QStringLiteral("isJavaScriptFunction"), true);

            const int formalParams = propFunction->getLength();
            if (propFunction->isConstructor()) {
                methodObject.insert(QStringLiteral("isConstructor"), true);

                QString ctorName;
                if (info.name.at(0).isUpper()) {
                    ctorName = info.name;
                } else if (info.name == QStringLiteral("constructor")) {
                    if (name.endsWith(QStringLiteral("Prototype")))
                        ctorName = name.chopped(strlen("Prototype"));
                    else if (name.endsWith(QStringLiteral("PrototypeMember")))
                        ctorName = name.chopped(strlen("PrototypeMember"));
                    else
                        ctorName = name;

                    if (!ctorName.endsWith(QStringLiteral("Constructor")))
                        ctorName += QStringLiteral("Constructor");
                }

                methodObject.insert(
                            QStringLiteral("returnType"),
                            buildConstructor(prop, classes, seen, ctorName, &constructed));
            }

            QJsonArray arguments;
            for (int i = 0; i < formalParams; i++)
                arguments.append(QJsonObject {});

            methodObject.insert(QStringLiteral("arguments"), arguments);

            methods.append(methodObject);

            continue;
        }

        // ...else it's just a property
        properties.append(defineProperty(prop, info));
    }

    for (const PropertyInfo &info : unRetrievedProperties) {
        QJSManagedValue prop = checkedProperty(
                    constructed.isUndefined() ? value : constructed, info.name);
        if (prop.engine()->hasError()) {
            qWarning() << "Cannot retrieve property " << info.name << "of" << name << constructed.toString();
            qWarning().noquote() << "    " << prop.engine()->catchError().toString();
        }

        properties.append(defineProperty(prop, info));
    }

    classObject[QStringLiteral("properties")] = properties;
    classObject[QStringLiteral("methods")] = methods;

    classes->append(classObject);

    return name;
}

static QString buildConstructor(const QJSManagedValue &constructor, QJsonArray *classes,
                                State *seen, const QString &name, QJSManagedValue *constructed)
{
    QJSEngine *engine = constructor.engine();

    // If the constructor appears in the global object, use the name from there.
    const QJSManagedValue globalObject(engine->globalObject(), engine);
    const auto infos = getPropertyInfos(globalObject);
    for (const auto &info : infos) {
        const QJSManagedValue member(globalObject.property(info.name), engine);
        if (member.strictlyEquals(constructor) && info.name != name)
            return buildConstructor(constructor, classes, seen, info.name, constructed);
    }

    if (name == QStringLiteral("Symbol"))
        return QStringLiteral("undefined"); // Cannot construct symbols with "new";

    if (name == QStringLiteral("URL")) {
        *constructed = QJSManagedValue(
                    constructor.callAsConstructor({ QJSValue(QStringLiteral("http://a.bc")) }),
                    engine);
    } else if (name == QStringLiteral("Promise")) {
        *constructed = QJSManagedValue(
                    constructor.callAsConstructor(
                        { engine->evaluate(QStringLiteral("(function() {})")) }),
                        engine);
    } else if (name == QStringLiteral("DataView")) {
        *constructed = QJSManagedValue(
                    constructor.callAsConstructor(
                        { engine->evaluate(QStringLiteral("new ArrayBuffer()")) }),
                        engine);
    } else if (name == QStringLiteral("Proxy")) {
        *constructed = QJSManagedValue(constructor.callAsConstructor(
                                          { engine->newObject(), engine->newObject() }), engine);
    } else {
        *constructed = QJSManagedValue(constructor.callAsConstructor(), engine);
    }

    if (engine->hasError()) {
        qWarning() << "Calling constructor" << name << "failed";
        qWarning().noquote() << "    " << engine->catchError().toString();
        return QString();
    } else if (name.isEmpty()) {
        Q_UNREACHABLE();
    }

    auto it = seen->constructors.find(name);
    if (it == seen->constructors.end()) {
        seen->constructors.insert(name, constructor.toJSValue());
        return buildClass(*constructed, classes, seen, name);
    } else if (!constructor.strictlyEquals(QJSManagedValue(*it, constructor.engine()))) {
        qWarning() << "Two constructors of the same name seen:" << name;
    }
    return name;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QStringList args = app.arguments();

    if (args.size() != 2) {
        qWarning().noquote() << app.applicationName() << "[output json path]";
        return 1;
    }

    QString fileName = args.at(1);

    QJSEngine engine;
    engine.installExtensions(QJSEngine::AllExtensions);

    QJsonArray classesArray;
    State seen;

    // object. Do this first to claim the "Object" name for the prototype.
    buildClass(QJSManagedValue(engine.newObject(), &engine), &classesArray, &seen,
               QStringLiteral("object"));


    buildClass(QJSManagedValue(engine.globalObject(), &engine), &classesArray, &seen,
               QStringLiteral("GlobalObject"));

    // Add JS types, in case they aren't used anywhere.


    // function
    buildClass(QJSManagedValue(engine.evaluate(QStringLiteral("(function() {})")), &engine),
               &classesArray, &seen, QStringLiteral("function"));

    // string
    buildClass(QJSManagedValue(QStringLiteral("s"), &engine), &classesArray, &seen,
               QStringLiteral("string"));

    // undefined
    buildClass(QJSManagedValue(QJSPrimitiveUndefined(), &engine), &classesArray, &seen,
               QStringLiteral("undefined"));

    // number
    buildClass(QJSManagedValue(QJSPrimitiveValue(1.1), &engine), &classesArray, &seen,
               QStringLiteral("number"));

    // boolean
    buildClass(QJSManagedValue(QJSPrimitiveValue(true), &engine), &classesArray, &seen,
               QStringLiteral("boolean"));

    // symbol
    buildClass(QJSManagedValue(engine.newSymbol(QStringLiteral("s")), &engine),
               &classesArray, &seen, QStringLiteral("symbol"));

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
