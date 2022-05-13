// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmlglobal_p.h>
#include <QtQml/private/qqmlmetatype_p.h>

#include <QtQml/qqmlengine.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

bool QQmlValueTypeProvider::initValueType(QMetaType metaType, QVariant &dst)
{
    if (!metaType.isValid())
        return false;
    dst = QVariant(metaType);
    return true;
}

bool QQmlValueTypeProvider::createValueType(QMetaType metaType, const QJSValue &s, QVariant &data)
{
    const QQmlType qmlType = QQmlMetaType::qmlType(metaType);
    if (auto valueTypeFunction = qmlType.createValueTypeFunction()) {
        QVariant result = valueTypeFunction(s);
        if (result.metaType() == metaType) {
            data = std::move(result);
            return true;
        }
    }

    return false;
}

bool QQmlValueTypeProvider::equalValueType(QMetaType metaType, const void *lhs, const QVariant &rhs)
{
    Q_ASSERT(lhs);
    return metaType.equals(lhs, rhs.constData());
}

bool QQmlValueTypeProvider::readValueType(QMetaType metaType, const QVariant  &src, void *dst)
{
    Q_ASSERT(dst);
    if (!metaType.isValid()
            || (src.metaType() == metaType && metaType.equals(src.constData(), dst))) {
        return false;
    }

    metaType.destruct(dst);
    metaType.construct(dst, src.metaType() == metaType ? src.constData() : nullptr);
    return true;
}

bool QQmlValueTypeProvider::writeValueType(QMetaType metaType, const void *src, QVariant &dst)
{
    Q_ASSERT(src);
    if (!metaType.isValid()
            || (dst.metaType() == metaType && metaType.equals(src, dst.constData()))) {
        return false;
    }

    dst = QVariant(metaType, src);
    return true;
}

Q_GLOBAL_STATIC(QQmlValueTypeProvider, valueTypeProvider)

Q_AUTOTEST_EXPORT QQmlValueTypeProvider *QQml_valueTypeProvider()
{
    return valueTypeProvider();
}

QQmlColorProvider::~QQmlColorProvider() {}
QVariant QQmlColorProvider::colorFromString(const QString &, bool *ok) { if (ok) *ok = false; return QVariant(); }
unsigned QQmlColorProvider::rgbaFromString(const QString &, bool *ok) { if (ok) *ok = false; return 0; }
QVariant QQmlColorProvider::fromRgbF(double, double, double, double) { return QVariant(); }
QVariant QQmlColorProvider::fromHslF(double, double, double, double) { return QVariant(); }
QVariant QQmlColorProvider::fromHsvF(double, double, double, double) { return QVariant(); }
QVariant QQmlColorProvider::lighter(const QVariant &, qreal) { return QVariant(); }
QVariant QQmlColorProvider::darker(const QVariant &, qreal) { return QVariant(); }
QVariant QQmlColorProvider::alpha(const QVariant &, qreal)
{
    return QVariant();
}
QVariant QQmlColorProvider::tint(const QVariant &, const QVariant &) { return QVariant(); }

static QQmlColorProvider *colorProvider = nullptr;

Q_QML_PRIVATE_EXPORT QQmlColorProvider *QQml_setColorProvider(QQmlColorProvider *newProvider)
{
    QQmlColorProvider *old = colorProvider;
    colorProvider = newProvider;
    return old;
}

static QQmlColorProvider **getColorProvider(void)
{
    if (colorProvider == nullptr) {
        qWarning() << "Warning: QQml_colorProvider: no color provider has been set!";
        static QQmlColorProvider nullColorProvider;
        colorProvider = &nullColorProvider;
    }

    return &colorProvider;
}

Q_AUTOTEST_EXPORT QQmlColorProvider *QQml_colorProvider(void)
{
    static QQmlColorProvider **providerPtr = getColorProvider();
    return *providerPtr;
}


QQmlGuiProvider::~QQmlGuiProvider() {}
QQmlApplication *QQmlGuiProvider::application(QObject *parent)
{
    return new QQmlApplication(parent);
}
QStringList QQmlGuiProvider::fontFamilies() { return QStringList(); }
bool QQmlGuiProvider::openUrlExternally(const QUrl &) { return false; }

QObject *QQmlGuiProvider::inputMethod()
{
    // We don't have any input method code by default
    QObject *o = new QObject();
    o->setObjectName(QStringLiteral("No inputMethod available"));
    QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    return o;
}

QObject *QQmlGuiProvider::styleHints()
{
    QObject *o = new QObject();
    o->setObjectName(QStringLiteral("No styleHints available"));
    QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    return o;
}

QString QQmlGuiProvider::pluginName() const { return QString(); }

static QQmlGuiProvider *guiProvider = nullptr;

Q_QML_PRIVATE_EXPORT QQmlGuiProvider *QQml_setGuiProvider(QQmlGuiProvider *newProvider)
{
    QQmlGuiProvider *old = guiProvider;
    guiProvider = newProvider;
    return old;
}

static QQmlGuiProvider **getGuiProvider(void)
{
    if (guiProvider == nullptr) {
        static QQmlGuiProvider nullGuiProvider; //Still provides an application with no GUI support
        guiProvider = &nullGuiProvider;
    }

    return &guiProvider;
}

Q_AUTOTEST_EXPORT QQmlGuiProvider *QQml_guiProvider(void)
{
    static QQmlGuiProvider **providerPtr = getGuiProvider();
    return *providerPtr;
}

//Docs in qqmlengine.cpp
QQmlApplication::QQmlApplication(QObject *parent)
    : QObject(*(new QQmlApplicationPrivate),parent)
{
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
            this, SIGNAL(aboutToQuit()));
    connect(QCoreApplication::instance(), SIGNAL(applicationNameChanged()),
            this, SIGNAL(nameChanged()));
    connect(QCoreApplication::instance(), SIGNAL(applicationVersionChanged()),
            this, SIGNAL(versionChanged()));
    connect(QCoreApplication::instance(), SIGNAL(organizationNameChanged()),
            this, SIGNAL(organizationChanged()));
    connect(QCoreApplication::instance(), SIGNAL(organizationDomainChanged()),
            this, SIGNAL(domainChanged()));
}

QQmlApplication::QQmlApplication(QQmlApplicationPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
            this, SIGNAL(aboutToQuit()));
    connect(QCoreApplication::instance(), SIGNAL(applicationNameChanged()),
            this, SIGNAL(nameChanged()));
    connect(QCoreApplication::instance(), SIGNAL(applicationVersionChanged()),
            this, SIGNAL(versionChanged()));
    connect(QCoreApplication::instance(), SIGNAL(organizationNameChanged()),
            this, SIGNAL(organizationChanged()));
    connect(QCoreApplication::instance(), SIGNAL(organizationDomainChanged()),
            this, SIGNAL(domainChanged()));
}

QStringList QQmlApplication::args()
{
    Q_D(QQmlApplication);
    if (!d->argsInit) {
        d->argsInit = true;
        d->args = QCoreApplication::arguments();
    }
    return d->args;
}

QString QQmlApplication::name() const
{
    return QCoreApplication::instance()->applicationName();
}

QString QQmlApplication::version() const
{
    return QCoreApplication::instance()->applicationVersion();
}

QString QQmlApplication::organization() const
{
    return QCoreApplication::instance()->organizationName();
}

QString QQmlApplication::domain() const
{
    return QCoreApplication::instance()->organizationDomain();
}

void QQmlApplication::setName(const QString &arg)
{
    QCoreApplication::instance()->setApplicationName(arg);
}

void QQmlApplication::setVersion(const QString &arg)
{
    QCoreApplication::instance()->setApplicationVersion(arg);
}

void QQmlApplication::setOrganization(const QString &arg)
{
    QCoreApplication::instance()->setOrganizationName(arg);
}

void QQmlApplication::setDomain(const QString &arg)
{
    QCoreApplication::instance()->setOrganizationDomain(arg);
}

QT_END_NAMESPACE

#include "moc_qqmlglobal_p.cpp"
