// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qandroidquickviewembedding_p.h>

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnitypes.h>
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

namespace QtAndroidQuickViewEmbedding
{
    void createQuickView(JNIEnv*, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference)
    {
        static_assert (sizeof(jlong) >= sizeof(void*),
                      "Insufficient size of Java type to hold the c++ pointer");
        const QUrl qmlUrl(QJniObject(qmlUri).toString());
        QMetaObject::invokeMethod(qApp, [qtViewObject = QJniObject(nativeWindow),
                                        parentWindowReference,
                                        width,
                                        height,
                                        qmlUrl] {
            QWindow *parentWindow = reinterpret_cast<QWindow *>(parentWindowReference);
            QQuickView* view = new QQuickView(parentWindow);
            view->setSource(qmlUrl);
            view->setColor(QColor(Qt::transparent));
            view->setWidth(width);
            view->setHeight(height);
            const QtJniTypes::QtWindow window = reinterpret_cast<jobject>(view->winId());
            qtViewObject.callMethod<void>("addQtWindow",
                                          window,
                                          reinterpret_cast<jlong>(view),
                                          parentWindowReference);
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

    bool registerNatives(QJniEnvironment& env) {
        return env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtQuickView>::className(),
                                         {Q_JNI_NATIVE_SCOPED_METHOD(createQuickView,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(setRootObjectProperty,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(getRootObjectProperty,
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
