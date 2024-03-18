// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qandroidquickviewembedding_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qjnitypes.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtDelegate, "org/qtproject/qt/android/QtEmbeddedContextDelegate");
Q_DECLARE_JNI_CLASS(QtQuickView, "org/qtproject/qt/android/QtQuickView");
Q_DECLARE_JNI_CLASS(QtWindow, "org/qtproject/qt/android/QtWindow");
Q_DECLARE_JNI_CLASS(View, "android/view/View");

Q_DECLARE_JNI_CLASS(Void, "java/lang/Void");
Q_DECLARE_JNI_CLASS(Integer, "java/lang/Integer");
Q_DECLARE_JNI_CLASS(Double, "java/lang/Double");
Q_DECLARE_JNI_CLASS(Float, "java/lang/Float");
Q_DECLARE_JNI_CLASS(Boolean, "java/lang/Boolean");
Q_DECLARE_JNI_CLASS(String, "java/lang/String");
Q_DECLARE_JNI_CLASS(Class, "java/lang/Class");

namespace QtAndroidQuickViewEmbedding
{
    void createQuickView(JNIEnv*, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference, QtJniTypes::StringArray qmlImportPaths)
    {
        static_assert (sizeof(jlong) >= sizeof(void*),
                      "Insufficient size of Java type to hold the c++ pointer");
        const QUrl qmlUrl(QJniObject(qmlUri).toString());

        QStringList importPaths;
        if (qmlImportPaths.isValid()) {
            QJniArray<QtJniTypes::String> importPathsArray(qmlImportPaths);
            importPaths.reserve(importPathsArray.size());
            for (int i = 0; i < importPathsArray.size(); ++i)
                importPaths << importPathsArray.at(i).toString();
        }

        QMetaObject::invokeMethod(qApp, [qtViewObject = QJniObject(nativeWindow),
                                        parentWindowReference,
                                        width,
                                        height,
                                        qmlUrl,
                                        importPaths] {
            QWindow *parentWindow = reinterpret_cast<QWindow *>(parentWindowReference);
            QQuickView *view = new QQuickView(parentWindow);
            QQmlEngine *engine = view->engine();
            new SignalHelper(view);
            QObject::connect(view, &QQuickView::statusChanged,
                             [qtViewObject](QQuickView::Status status) {
                                 qtViewObject.callMethod<void>("handleStatusChange", status);
                             });
            view->setResizeMode(QQuickView::SizeRootObjectToView);
            view->setColor(QColor(Qt::transparent));
            view->setWidth(width);
            view->setHeight(height);
            for (const QString &path : importPaths)
                engine->addImportPath(path);

            const QtJniTypes::QtWindow window = reinterpret_cast<jobject>(view->winId());
            qtViewObject.callMethod<void>("addQtWindow",
                                          window,
                                          reinterpret_cast<jlong>(view),
                                          parentWindowReference);
            view->setSource(qmlUrl);
        });
    }

    void setRootObjectProperty(JNIEnv *env, jobject object, jlong windowReference,
                               jstring propertyName, jobject value)
    {
        Q_UNUSED(env);
        Q_UNUSED(object);

        QQuickItem *rootObject = reinterpret_cast<QQuickView *>(windowReference)->rootObject();
        if (!rootObject) {
            qWarning() << "QtQuickView instance does not own a root object.";
            return;
        }

        const QString property = QJniObject(propertyName).toString();
        const QMetaObject *rootMetaObject = rootObject->metaObject();
        int propertyIndex = rootMetaObject->indexOfProperty(qPrintable(property));
        if (propertyIndex < 0) {
            qWarning("Property %s does not exist in the root QML object.", qPrintable(property));
            return;
        }

        QMetaProperty metaProperty = rootMetaObject->property(propertyIndex);
        const QJniObject propertyValue(value);
        const QByteArray valueClassname = propertyValue.className();

        if (valueClassname == QtJniTypes::Traits<QtJniTypes::String>::className())
            metaProperty.write(rootObject, propertyValue.toString());
        else if (valueClassname == QtJniTypes::Traits<QtJniTypes::Integer>::className())
            metaProperty.write(rootObject, propertyValue.callMethod<jint>("intValue"));
        else if (valueClassname == QtJniTypes::Traits<QtJniTypes::Double>::className())
            metaProperty.write(rootObject, propertyValue.callMethod<jdouble>("doubleValue"));
        else if (valueClassname == QtJniTypes::Traits<QtJniTypes::Float>::className())
            metaProperty.write(rootObject, propertyValue.callMethod<jfloat>("floatValue"));
        else if (valueClassname == QtJniTypes::Traits<QtJniTypes::Boolean>::className())
            metaProperty.write(rootObject, propertyValue.callMethod<jboolean>("booleanValue"));
        else
            qWarning("Setting the property type of %s is not supported.", valueClassname.data());
    }

    jobject getRootObjectProperty(JNIEnv *env, jobject object, jlong windowReference,
                                  jstring propertyName)
    {
        Q_UNUSED(object);
        Q_ASSERT(env);

        const QString property = QJniObject(propertyName).toString();
        QQuickView *view = reinterpret_cast<QQuickView *>(windowReference);
        QQuickItem *rootObject = view->rootObject();
        if (!rootObject) {
            qWarning("Cannot read property %s as the QtQuickView instance (%s)"
                     "does not own a root object.",
                     qPrintable(property),
                     qPrintable(view->source().toString()));
            return nullptr;
        }

        const QMetaObject *rootMetaObject = rootObject->metaObject();
        int propertyIndex = rootMetaObject->indexOfProperty(property.toUtf8().constData());
        if (propertyIndex < 0) {
            qWarning("Cannot read property %s as it does not exist in the root QML object.",
                     qPrintable(property));
            return nullptr;
        }

        QMetaProperty metaProperty = rootMetaObject->property(propertyIndex);
        QVariant propertyValue = metaProperty.read(rootObject);
        const int propertyTypeId = propertyValue.typeId();

        switch (propertyTypeId) {
        case QMetaType::Type::Int:
            return env->NewLocalRef(
                QJniObject::construct<QtJniTypes::Integer>(get<int>(std::move(propertyValue)))
                    .object());
        case QMetaType::Type::Double:
            return env->NewLocalRef(
                QJniObject::construct<QtJniTypes::Double>(get<double>(std::move(propertyValue)))
                    .object());
        case QMetaType::Type::Float:
            return env->NewLocalRef(
                QJniObject::construct<QtJniTypes::Float>(get<float>(std::move(propertyValue)))
                    .object());
        case QMetaType::Type::Bool:
            return env->NewLocalRef(
                QJniObject::construct<QtJniTypes::Boolean>(get<bool>(std::move(propertyValue)))
                    .object());
        case QMetaType::Type::QString:
            return env->NewLocalRef(
                QJniObject::fromString(get<QString>(std::move(propertyValue))).object());
        default:
            qWarning("Property %s cannot be converted to a supported Java data type.",
                     qPrintable(property));
        }

        return nullptr;
    }

    int addRootObjectSignalListener(JNIEnv *env, jobject, jlong windowReference, jstring signalName,
                                   jclass argType, jobject listener)
    {
        Q_ASSERT(env);
        static QHash<QByteArray, int> javaToQMetaType = {
            { "java/lang/Void", QMetaType::Type::Void },
            { "java/lang/String", QMetaType::Type::QString },
            { "java/lang/Integer", QMetaType::Type::Int },
            { "java/lang/Double", QMetaType::Type::Double },
            { "java/lang/Float", QMetaType::Type::Float },
            { "java/lang/Boolean", QMetaType::Type::Bool }
        };

        QQuickView *view = reinterpret_cast<QQuickView *>(windowReference);
        if (!view) {
            qWarning() << "QtQuickView is not loaded or ready yet.";
            return -1;
        }
        QQuickItem *rootObject = view->rootObject();
        if (!rootObject) {
            qWarning() << "QtQuickView instance does not own a root object.";
            return -1;
        }

        SignalHelper *signalHelper = view->findChild<SignalHelper *>();
        const QByteArray javaArgClass = QJniObject(argType).className();
        const char *qArgName =
                QMetaType(javaToQMetaType.value(javaArgClass, QMetaType::Type::UnknownType)).name();
        const QString signalMethodName = QJniObject(signalName).toString();

        const QMetaObject *metaObject = rootObject->metaObject();
        int signalIndex = -1;
        int propertyIndex = -1;

        QByteArray signalSignature = QMetaObject::normalizedSignature(qPrintable(
                QStringLiteral("%1(%2)").arg(signalMethodName).arg(QLatin1StringView(qArgName))));
        signalIndex = metaObject->indexOfSignal(signalSignature.constData());

        // Try to check if the signal is a parameterless notifier of a property
        // or a property name itself.
        if (signalIndex == -1) {
            signalSignature = QMetaObject::normalizedSignature(
                    qPrintable(QStringLiteral("%1()").arg(signalMethodName)));
            for (int i = 0; i < metaObject->propertyCount(); ++i) {
                QMetaProperty metaProperty = metaObject->property(i);
                QMetaMethod notifyMethod = metaProperty.notifySignal();

                if (signalSignature == notifyMethod.methodSignature()) {
                    signalIndex = metaObject->property(i).notifySignalIndex();
                    propertyIndex = i;
                    break;
                } else if (signalMethodName == QLatin1StringView(metaProperty.name())) {
                    signalIndex = metaObject->property(i).notifySignalIndex();
                    signalSignature = notifyMethod.methodSignature();
                    propertyIndex = i;
                    break;
                }
            }
        }

        if (signalIndex == -1)
            return -1;

        const QMetaObject *helperMetaObject = signalHelper->metaObject();
        QByteArray helperSignalSignature = signalSignature;
        helperSignalSignature.replace(0, signalSignature.indexOf('('), "forwardSignal");
        int helperSlotIndex = helperMetaObject->indexOfSlot(helperSignalSignature.constData());
        if (helperSlotIndex == -1)
            return -1;

        // Return the id if the signal is already connected to the same listener.
        QJniObject listenerJniObject(listener);
        if (signalHelper->listenersMap.contains(signalSignature)) {
            auto listenerInfos = signalHelper->listenersMap.values(signalSignature);
            auto isSameListener = [listenerJniObject](const SignalHelper::ListenerInfo &listenerInfo) {
                return listenerInfo.listener == listenerJniObject;
            };
            auto iterator = std::find_if(listenerInfos.constBegin(),
                                         listenerInfos.constEnd(),
                                         isSameListener);
            if (iterator != listenerInfos.end()) {
                qWarning("Signal listener with the ID of %i is already connected to %s signal.",
                         iterator->id,
                         signalSignature.constData());
                return iterator->id;
            }
        }

        QMetaMethod signalMethod = metaObject->method(signalIndex);
        QMetaMethod signalForwarderMethod = helperMetaObject->method(helperSlotIndex);
        signalHelper->connectionHandleCounter++;

        QMetaObject::Connection connection;
        if (signalHelper->listenersMap.contains(signalSignature)) {
            connection = signalHelper
                             ->connections[signalHelper->listenersMap.value(signalSignature).id];
        } else {
            connection = QObject::connect(rootObject,
                                          signalMethod,
                                          signalHelper,
                                          signalForwarderMethod);
        }

        SignalHelper::ListenerInfo listenerInfo;
        listenerInfo.listener = listenerJniObject;
        listenerInfo.javaArgType = javaArgClass;
        listenerInfo.propertyIndex = propertyIndex;
        listenerInfo.signalSignature = signalSignature;
        listenerInfo.id = signalHelper->connectionHandleCounter;

        signalHelper->listenersMap.insert(signalSignature, listenerInfo);
        signalHelper->connections.insert(listenerInfo.id, connection);

        return listenerInfo.id;
    }

    bool removeRootObjectSignalListener(JNIEnv *, jobject, jlong windowReference,
                                       jint signalListenerId)
    {
        QQuickView *view = reinterpret_cast<QQuickView *>(windowReference);
        QQuickItem *rootObject = view->rootObject();
        if (!rootObject) {
            qWarning() << "QtQuickView instance does not own a root object.";
            return false;
        }

        SignalHelper *signalHelper = view->findChild<SignalHelper *>();
        if (!signalHelper->connections.contains(signalListenerId))
            return false;

        QByteArray signalSignature;
        for (auto listenerInfoIter = signalHelper->listenersMap.begin();
             listenerInfoIter != signalHelper->listenersMap.end();) {
            if (listenerInfoIter->id == signalListenerId) {
                signalSignature = listenerInfoIter->signalSignature;
                signalHelper->listenersMap.erase(listenerInfoIter);
                break;
            } else {
                ++listenerInfoIter;
            }
        }

        // disconnect if its the last listener associated with the signal signatures
        if (!signalHelper->listenersMap.contains(signalSignature))
            rootObject->disconnect(signalHelper->connections.value(signalListenerId));

        signalHelper->connections.remove(signalListenerId);
        return true;
    }

    void SignalHelper::forwardSignal()
    {
        invokeListener(sender(), senderSignalIndex(), QVariant());
    }

    void SignalHelper::forwardSignal(int signalValue)
    {
        invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
    }

    void SignalHelper::forwardSignal(bool signalValue)
    {
        invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
    }

    void SignalHelper::forwardSignal(double signalValue)
    {
        invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
    }

    void SignalHelper::forwardSignal(float signalValue)
    {
        invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
    }

    void SignalHelper::forwardSignal(QString signalValue)
    {
        invokeListener(sender(), senderSignalIndex(), QVariant(signalValue));
    }

    void SignalHelper::invokeListener(QObject *sender, int senderSignalIndex, QVariant signalValue)
    {
        using namespace QtJniTypes;

        const QMetaObject *metaObject = sender->metaObject();
        const QMetaMethod signalMethod = metaObject->method(senderSignalIndex);

        for (auto listenerInfoIter = listenersMap.constFind(signalMethod.methodSignature());
             listenerInfoIter != listenersMap.constEnd() &&
             listenerInfoIter.key() == signalMethod.methodSignature();
             ++listenerInfoIter) {
            const ListenerInfo listenerInfo = *listenerInfoIter;
            const QByteArray javaArgType = listenerInfo.javaArgType;
            QJniObject jSignalMethodName =
                    QJniObject::fromString(QLatin1StringView(signalMethod.name()));

            if (listenerInfo.propertyIndex != -1 && javaArgType != Traits<Void>::className())
                signalValue = metaObject->property(listenerInfo.propertyIndex).read(sender);

            int valueTypeId = signalValue.typeId();
            QJniObject jValue;

            switch (valueTypeId) {
            case QMetaType::Type::UnknownType:
                break;
            case QMetaType::Type::Int:
                jValue = qVariantToJniObject<Integer,jint>(signalValue);
                break;
            case QMetaType::Type::Double:
                jValue = qVariantToJniObject<Double,jdouble>(signalValue);
                break;
            case QMetaType::Type::Float:
                jValue = qVariantToJniObject<Float,jfloat>(signalValue);
                break;
            case QMetaType::Type::Bool:
                jValue = qVariantToJniObject<Boolean,jboolean>(signalValue);
                break;
            case QMetaType::Type::QString:
                jValue = QJniObject::fromString(get<QString>(std::move(signalValue)));
                break;
            default:
                qWarning("Mismatching argument types between QML signal (%s) and the Java function "
                         "(%s). Sending null as argument.",
                         signalMethod.methodSignature().constData(), javaArgType.constData());
            }

            QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                [listenerInfo, jSignalMethodName, jValue]() {
                    listenerInfo.listener.callMethod<void, jstring, jobject>("onSignalEmitted",
                                                            jSignalMethodName.object<jstring>(),
                                                            jValue.object());
                });
        }
    }

    bool registerNatives(QJniEnvironment& env) {
        return env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtQuickView>::className(),
                                         {Q_JNI_NATIVE_SCOPED_METHOD(createQuickView,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(setRootObjectProperty,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(getRootObjectProperty,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(addRootObjectSignalListener,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(removeRootObjectSignalListener,
                                                                     QtAndroidQuickViewEmbedding)});
    }
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    Q_UNUSED(vm)
    Q_UNUSED(reserved)

    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QJniEnvironment env;
    if (!env.isValid())
        return JNI_ERR;
    if (!QtAndroidQuickViewEmbedding::registerNatives(env))
        return JNI_ERR;
    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
