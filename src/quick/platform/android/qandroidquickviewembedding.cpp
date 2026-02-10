// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qandroidtypeconverter_p.h>
#include <QtCore/private/qandroidtypes_p.h>
#include <QtQuick/private/qandroidquickviewembedding_p.h>
#include <QtQuick/private/qandroidviewsignalmanager_p.h>
#include <QtCore/private/qmetaobject_p.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qjnitypes.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <functional>
#include <jni.h>
#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtDelegate, "org/qtproject/qt/android/QtEmbeddedContextDelegate");
Q_DECLARE_JNI_CLASS(QtQuickView, "org/qtproject/qt/android/QtQuickView");
Q_DECLARE_JNI_CLASS(QtWindow, "org/qtproject/qt/android/QtWindow");
Q_DECLARE_JNI_CLASS(View, "android/view/View");

namespace QtAndroidQuickViewEmbedding
{
    constexpr const char *uninitializedViewMessage = "because QtQuickView is not loaded or ready yet.";

    static void onQQuickViewStatusChanged(const QJniObject &qtViewObject,
                                          QAndroidQuickView::Status status)
    {
        auto future = QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                [qtViewObject, status] {
                    qtViewObject.callMethod<void>("handleStatusChange", status);
                }, QDeadlineTimer(1000));
        future.waitForFinished(); // Wait for the user to handle status change.
    }

    void createQuickView(JNIEnv *, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference, jlong viewReference,
                         const QJniArray<jstring> &qmlImportPaths)
    {
        static_assert (sizeof(jlong) >= sizeof(void*),
                      "Insufficient size of Java type to hold the c++ pointer");
        const QUrl qmlUrl(QJniObject(qmlUri).toString());

        const QStringList importPaths = qmlImportPaths.toContainer();
        QMetaObject::invokeMethod(qApp, [qtViewObject = QJniObject(nativeWindow),
                                        parentWindowReference,
                                        viewReference,
                                        width,
                                        height,
                                        qmlUrl,
                                        importPaths] {
            // If the view does not exists (viewReference==0) we should create and set it up.
            // Else we only reset the source of the view.
            QAndroidQuickView *view = reinterpret_cast<QAndroidQuickView *>(viewReference);
            if (!view) {
                QWindow *parentWindow = reinterpret_cast<QWindow *>(parentWindowReference);
                view = new QAndroidQuickView(parentWindow);
                QObject::connect(
                        view, &QAndroidQuickView::statusChanged,
                        std::bind(&onQQuickViewStatusChanged, qtViewObject, std::placeholders::_1));
                view->setResizeMode(QAndroidQuickView::SizeRootObjectToView);
                view->setColor(QColor(Qt::transparent));
                view->setWidth(width);
                view->setHeight(height);
                QQmlEngine *engine = view->engine();
                for (const QString &path : importPaths)
                    engine->addImportPath(path);

                QObject::connect(engine, &QQmlEngine::quit, QCoreApplication::instance(),
                                 &QCoreApplication::quit);

                const QtJniTypes::QtWindow window = reinterpret_cast<jobject>(view->winId());
                qtViewObject.callMethod<void>("addQtWindow",
                                              window,
                                              reinterpret_cast<jlong>(view),
                                              parentWindowReference);
            }
            view->setSource(qmlUrl);
        });
    }

    std::pair<QAndroidQuickView *, QQuickItem *> getViewAndRootObject(jlong windowReference)
    {
        QAndroidQuickView *view = reinterpret_cast<QAndroidQuickView *>(windowReference);
        QQuickItem *rootObject = Q_LIKELY(view) ? view->rootObject() : nullptr;
        return std::make_pair(view, rootObject);
    }

    void setRootObjectProperty(JNIEnv *env, jobject object, jlong windowReference,
                               jstring propertyName, jobject value)
    {
        Q_UNUSED(env);
        Q_UNUSED(object);

        auto [_, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot set property %s %s", qPrintable(QJniObject(propertyName).toString()),
                     uninitializedViewMessage);
            return;
        }

        const QString property = QJniObject(propertyName).toString();
        const QMetaObject *rootMetaObject = rootObject->metaObject();
        int propertyIndex = rootMetaObject->indexOfProperty(qPrintable(property));
        if (propertyIndex < 0) {
            qWarning("Property %s does not exist in the root QML object.", qPrintable(property));
            return;
        }

        const QJniObject propertyValue(value);
        const QVariant variantToWrite = QAndroidTypeConverter::toQVariant(propertyValue);

        if (!variantToWrite.isValid()) {
            qWarning("Setting the property type of %s is not supported.",
                     propertyValue.className().data());
        } else {
            QMetaObject::invokeMethod(rootObject,
                [metaProperty = rootMetaObject->property(propertyIndex),
                 rootObject = rootObject,
                 variantToWrite] {
                    metaProperty.write(rootObject, variantToWrite);
                });
        }
    }

    jobject getRootObjectProperty(JNIEnv *env, jobject object, jlong windowReference,
                                  jstring propertyName)
    {
        Q_UNUSED(object);
        Q_ASSERT(env);

        const QString property = QJniObject(propertyName).toString();
        auto [_, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot get property %s %s", qPrintable(property), uninitializedViewMessage);
            return nullptr;
        }

        const QMetaObject *rootMetaObject = rootObject->metaObject();
        int propertyIndex = rootMetaObject->indexOfProperty(property.toUtf8().constData());
        if (propertyIndex < 0) {
            qWarning("Cannot get property %s as it does not exist in the root QML object.",
                     qPrintable(property));
            return nullptr;
        }

        QMetaProperty metaProperty = rootMetaObject->property(propertyIndex);
        QVariant propertyValue;
        if (QCoreApplication::instance()->thread()->isCurrentThread()) {
            propertyValue = metaProperty.read(rootObject);
        } else {
            QMetaObject::invokeMethod(rootObject,
                [&propertyValue, &metaProperty, rootObject = rootObject] {
                    propertyValue = metaProperty.read(rootObject);
                }, Qt::BlockingQueuedConnection);
        }
        jobject jObject = QAndroidTypeConverter::toJavaObject(propertyValue, env);
        if (!jObject) {
            qWarning("Property %s cannot be converted to a supported Java data type.",
                     qPrintable(property));
        }
        return jObject;
    }

    bool addRootObjectSignalListener(JNIEnv *env, jobject, jlong windowReference,
                                     jstring signalName, QJniArray<jclass> argTypes,
                                     jobject listener, jint id)
    {
        Q_ASSERT(env);

        auto [view, _] = getViewAndRootObject(windowReference);
        if (!view) {
            qWarning("Cannot connect to signal %s %s",
                     qPrintable(QJniObject(signalName).toString()), uninitializedViewMessage);
            return false;
        }

        QAndroidViewSignalManager *signalManager = view->signalManager();
        return signalManager->addConnection(QJniObject(signalName).toString(), argTypes,
                                            QJniObject(listener), id);
    }

    bool removeRootObjectSignalListener(JNIEnv *, jobject, jlong windowReference,
                                       jint signalListenerId)
    {
        auto [view, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot disconnect the signal connection with id: %i %s", signalListenerId,
                     uninitializedViewMessage);
            return false;
        }
        view->signalManager()->removeConnection(signalListenerId);
        return true;
    }

    QVariant jobjectToVariant(QMetaType::Type type, jobject &obj)
    {
        switch (type) {
        case QMetaType::Bool:
            return QVariant::fromValue(
                    QtJniTypes::Boolean::construct(obj).callMethod<bool>("booleanValue"));
            break;
        case QMetaType::Int:
            return QVariant::fromValue(
                    QtJniTypes::Integer::construct(obj).callMethod<int>("intValue"));
            break;
        case QMetaType::Double:
            return QVariant::fromValue(
                    QtJniTypes::Double::construct(obj).callMethod<double>("doubleValue"));
            break;
        case QMetaType::Float:
            return QVariant::fromValue(
                    QtJniTypes::Float::construct(obj).callMethod<float>("floatValue"));
            break;
        case QMetaType::QString:
            return QVariant::fromValue(QJniObject(obj).toString());
            break;
        default:
            qWarning("Unsupported metatype: %s", QMetaType(type).name());
            return QVariant();
        }
    }

    QMetaMethod findMethod(const QString &name, int paramCount, const QMetaObject &object)
    {
        for (auto i = object.methodOffset(); i < object.methodCount(); ++i) {
            QMetaMethod method = object.method(i);
            const auto paramMatch = method.parameterCount() == paramCount;
            const auto nameMatch = method.name() == name.toUtf8();
            if (paramMatch && nameMatch)
                return method;
        }
        return QMetaMethod();
    }

    void invokeMethod(JNIEnv *, jobject, jlong viewReference, QtJniTypes::String methodName,
                      QJniArray<jobject> jniParams)
    {
        auto [_, rootObject] = getViewAndRootObject(viewReference);
        if (!rootObject) {
            qWarning() << "Cannot invoke QML method" << methodName.toString()
                       << "as the QML view has not been loaded yet.";
            return;
        }

        const auto paramCount = jniParams.size();
        QMetaMethod method =
                findMethod(methodName.toString(), paramCount, *rootObject->metaObject());
        if (!method.isValid()) {
            qWarning() << "Failed to find method" << QJniObject(methodName).toString()
                       << "in QQuickView";
            return;
        }

        // Invoke and leave early if there are no params to pass on
        if (paramCount == 0) {
            method.invoke(rootObject, Qt::QueuedConnection);
            return;
        }

        QList<QVariant> variants;
        variants.reserve(jniParams.size());
        variants.emplace_back(QVariant{}); // "Data" for the return value

        for (auto i = 0; i < paramCount; ++i) {
            const auto type = method.parameterType(i);
            if (type == QMetaType::UnknownType) {
                qWarning("Unknown metatypes are not supported.");
                return;
            }

            jobject rawParam = jniParams.at(i);
            auto variant = variants.emplace_back(
                    jobjectToVariant(static_cast<QMetaType::Type>(type), rawParam));
            if (variant.isNull()) {
                auto className = QJniObject(rawParam).className();
                qWarning("Failed to convert param with class name '%s' to QVariant",
                         className.constData());
                return;
            }
        }

        // Initialize the data arrays for params, typenames and type conversion interfaces.
        // Note that this is adding an element, this is for the return value which is at idx 0.
        const int paramsCount = method.parameterCount() + 1;
        const auto paramTypes = std::make_unique<const char *[]>(paramsCount);
        const auto params = std::make_unique<const void *[]>(paramsCount);
        const auto metaTypes =
                std::make_unique<const QtPrivate::QMetaTypeInterface *[]>(paramsCount);

        // We're not expecting a return value, so index 0 can be all nulls.
        paramTypes[0] = nullptr;
        params[0] = nullptr;
        metaTypes[0] = nullptr;

        for (auto i = 1; i < variants.size(); ++i) {
            const auto &variant = variants.at(i);
            paramTypes[i] = variant.typeName();
            params[i] = variant.data();
            metaTypes[i] = variant.metaType().iface();
        }

        auto reason = QMetaMethodInvoker::invokeImpl(method,
                                                     rootObject,
                                                     Qt::QueuedConnection,
                                                     paramsCount,
                                                     params.get(),
                                                     paramTypes.get(),
                                                     metaTypes.get());

        if (reason != QMetaMethodInvoker::InvokeFailReason::None)
            qWarning() << "Failed to invoke function" << methodName.toString()
                       << ", Reason:" << int(reason);
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
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(invokeMethod,
                                                                     QtAndroidQuickViewEmbedding)});
    }
}

extern "C" Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
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
