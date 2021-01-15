/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLBUILTINFUNCTIONS_P_H
#define QQMLBUILTINFUNCTIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmlglobal_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qjsengine_p.h>
#include <private/qqmlplatform_p.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QtObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlApplication *application READ application CONSTANT)
    Q_PROPERTY(QQmlPlatform *platform READ platform CONSTANT)
    Q_PROPERTY(QObject *inputMethod READ inputMethod CONSTANT)
    Q_PROPERTY(QObject *styleHints READ styleHints CONSTANT)
    Q_PROPERTY(QJSValue callLater READ callLater CONSTANT)

#if QT_CONFIG(translation)
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage BINDABLE uiLanguageBindable)
#endif

    QML_NAMED_ELEMENT(Qt)
    QML_SINGLETON
    QML_EXTENDED_NAMESPACE(Qt)
    QML_ADDED_IN_VERSION(2, 0)

    Q_CLASSINFO("QML.StrictArguments", "true")

public:
    enum LoadingMode { Asynchronous = 0, Synchronous = 1 };
    Q_ENUM(LoadingMode);

    static QtObject *create(QQmlEngine *, QJSEngine *jsEngine);

    Q_INVOKABLE QJSValue include(const QString &url, const QJSValue &callback = QJSValue()) const;
    Q_INVOKABLE bool isQtObject(const QJSValue &value) const;

    Q_INVOKABLE QVariant color(const QString &name) const;
    Q_INVOKABLE QVariant rgba(double r, double g, double b, double a = 1) const;
    Q_INVOKABLE QVariant hsla(double h, double s, double l, double a = 1) const;
    Q_INVOKABLE QVariant hsva(double h, double s, double v, double a = 1) const;
    Q_INVOKABLE bool colorEqual(const QVariant &lhs, const QVariant &rhs) const;

    Q_INVOKABLE QRectF rect(double x, double y, double width, double height) const;
    Q_INVOKABLE QPointF point(double x, double y) const;
    Q_INVOKABLE QSizeF size(double width, double height) const;
    Q_INVOKABLE QVariant vector2d(double x, double y) const;
    Q_INVOKABLE QVariant vector3d(double x, double y, double z) const;
    Q_INVOKABLE QVariant vector4d(double x, double y, double z, double w) const;
    Q_INVOKABLE QVariant quaternion(double scalar, double x, double y, double z) const;

    Q_INVOKABLE QVariant matrix4x4() const;
    Q_INVOKABLE QVariant matrix4x4(double m11, double m12, double m13, double m14,
                                   double m21, double m22, double m23, double m24,
                                   double m31, double m32, double m33, double m34,
                                   double m41, double m42, double m43, double m44) const;
    Q_INVOKABLE QVariant matrix4x4(const QJSValue &value) const;

    Q_INVOKABLE QVariant lighter(const QJSValue &color, double factor = 1.5) const;
    Q_INVOKABLE QVariant darker(const QJSValue &color, double factor = 2.0) const;
    Q_INVOKABLE QVariant alpha(const QJSValue &baseColor, double value) const;
    Q_INVOKABLE QVariant tint(const QJSValue &baseColor, const QJSValue &tintColor) const;

    Q_INVOKABLE QString formatDate(const QDate &date, const QString &format) const;
    Q_INVOKABLE QString formatDate(const QDate &date, Qt::DateFormat format) const;

    Q_INVOKABLE QString formatTime(const QTime &time, const QString &format) const;
    Q_INVOKABLE QString formatTime(const QString &time, const QString &format) const;
    Q_INVOKABLE QString formatTime(const QTime &time, Qt::DateFormat format) const;
    Q_INVOKABLE QString formatTime(const QString &time, Qt::DateFormat format) const;

    Q_INVOKABLE QString formatDateTime(const QDateTime &date, const QString &format) const;
    Q_INVOKABLE QString formatDateTime(const QDateTime &date, Qt::DateFormat format) const;

#if QT_CONFIG(qml_locale)
    Q_INVOKABLE QString formatDate(const QDate &date, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatTime(const QTime &time, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatTime(const QString &time, const QLocale &locale = QLocale(),
                                   QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QString formatDateTime(const QDateTime &date, const QLocale &locale = QLocale(),
                                       QLocale::FormatType formatType = QLocale::ShortFormat) const;
    Q_INVOKABLE QLocale locale() const;
    Q_INVOKABLE QLocale locale(const QString &name) const;
#endif

    Q_INVOKABLE QUrl resolvedUrl(const QUrl &url) const;
    Q_INVOKABLE bool openUrlExternally(const QUrl &url) const;

    Q_INVOKABLE QVariant font(const QJSValue &fontSpecifier) const;
    Q_INVOKABLE QStringList fontFamilies() const;

    Q_INVOKABLE QString md5(const QString &data) const;
    Q_INVOKABLE QString btoa(const QString &data) const;
    Q_INVOKABLE QString atob(const QString &data) const;

    Q_INVOKABLE void quit() const;
    Q_INVOKABLE void exit(int retCode) const;

    Q_INVOKABLE QObject *createQmlObject(const QString &qml, QObject *parent,
                                         const QUrl &url = QUrl(QStringLiteral("inline"))) const;
    Q_INVOKABLE QQmlComponent *createComponent(const QUrl &url, QObject *parent) const;
    Q_INVOKABLE QQmlComponent *createComponent(
            const QUrl &url, QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous,
            QObject *parent = nullptr) const;

    Q_INVOKABLE QJSValue binding(const QJSValue &function) const;

    // We can't make this invokable as it uses actual varargs
    static QV4::ReturnedValue method_callLater(
            const QV4::FunctionObject *b, const QV4::Value *thisObject,
            const QV4::Value *argv, int argc);

#if QT_CONFIG(translation)
    QString uiLanguage() const;
    void setUiLanguage(const QString &uiLanguage);
    QBindable<QString> uiLanguageBindable();
#endif

    // Not const because created on first use, and parented to this.
    QQmlPlatform *platform();
    QQmlApplication *application();

    QObject *inputMethod() const;
    QObject *styleHints() const;
    QJSValue callLater() const;

private:
    friend struct QV4::ExecutionEngine;

    QtObject(QV4::ExecutionEngine *engine);

    QQmlEngine *qmlEngine() const { return m_engine->qmlEngine(); }
    QJSEngine *jsEngine() const { return m_engine->jsEngine(); }
    QV4::ExecutionEngine *v4Engine() const { return m_engine; }

    QQmlPlatform *m_platform = nullptr;
    QQmlApplication *m_application = nullptr;

    QV4::ExecutionEngine *m_engine = nullptr;
    QJSValue m_callLater;
};

namespace QV4 {

namespace Heap {

struct ConsoleObject : Object {
    void init();
};

#define QQmlBindingFunctionMembers(class, Member) \
    Member(class, Pointer, FunctionObject *, bindingFunction)
DECLARE_HEAP_OBJECT(QQmlBindingFunction, FunctionObject) {
    DECLARE_MARKOBJECTS(QQmlBindingFunction)
    void init(const QV4::FunctionObject *bindingFunction);
};

}

struct ConsoleObject : Object
{
    V4_OBJECT2(ConsoleObject, Object)

    static ReturnedValue method_error(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_log(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_info(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_profile(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_profileEnd(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_time(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_timeEnd(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_count(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_trace(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_warn(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_assert(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_exception(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

};

struct Q_QML_PRIVATE_EXPORT GlobalExtensions {
    static void init(Object *globalObject, QJSEngine::Extensions extensions);

#if QT_CONFIG(translation)
    static ReturnedValue method_qsTranslate(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTranslateNoOp(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTr(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTrNoOp(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTrId(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_qsTrIdNoOp(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
#endif
    static ReturnedValue method_gc(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

    // on String:prototype
    static ReturnedValue method_string_arg(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

};

struct QQmlBindingFunction : public QV4::FunctionObject
{
    V4_OBJECT2(QQmlBindingFunction, FunctionObject)

    Heap::FunctionObject *bindingFunction() const { return d()->bindingFunction; }
    QQmlSourceLocation currentLocation() const; // from caller stack trace
};

inline bool FunctionObject::isBinding() const
{
    return d()->vtable() == QQmlBindingFunction::staticVTable();
}

}

QT_END_NAMESPACE

#endif // QQMLBUILTINFUNCTIONS_P_H
