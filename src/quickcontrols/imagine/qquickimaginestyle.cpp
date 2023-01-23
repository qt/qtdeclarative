// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimaginestyle_p.h"

#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QString, GlobalPath, (QLatin1String("qrc:/qt-project.org/imports/QtQuick/Controls/Imagine/images/")))

static QString ensureSlash(const QString &path)
{
    const QChar slash = QLatin1Char('/');
    return path.endsWith(slash) ? path : path + slash;
}

QQuickImagineStyle::QQuickImagineStyle(QObject *parent)
    : QQuickAttachedPropertyPropagator(parent),
      m_path(*GlobalPath())
{
    init();
}

QQuickImagineStyle *QQuickImagineStyle::qmlAttachedProperties(QObject *object)
{
    return new QQuickImagineStyle(object);
}

QString QQuickImagineStyle::path() const
{
    return m_path;
}

void QQuickImagineStyle::setPath(const QString &path)
{
    m_explicitPath = true;
    if (m_path == path)
        return;

    m_path = path;
    propagatePath();

    emit pathChanged();
}

void QQuickImagineStyle::inheritPath(const QString &path)
{
    if (m_explicitPath || m_path == path)
        return;

    m_path = path;
    propagatePath();
    emit pathChanged();
}

void QQuickImagineStyle::propagatePath()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        QQuickImagineStyle *imagine = qobject_cast<QQuickImagineStyle *>(child);
        if (imagine)
            imagine->inheritPath(m_path);
    }
}

void QQuickImagineStyle::resetPath()
{
    if (!m_explicitPath)
        return;

    m_explicitPath = false;
    QQuickImagineStyle *imagine = qobject_cast<QQuickImagineStyle *>(attachedParent());
    inheritPath(imagine ? imagine->path() : *GlobalPath());
}

QUrl QQuickImagineStyle::url() const
{
    // Using ApplicationWindow as an example, its NinePatchImage url
    // was previously assigned like this:
    //
    // soruce: Imagine.path + "applicationwindow-background"
    //
    // If Imagine.path is set to ":/images" by the user, then the final URL would be:
    //
    // QUrl("file:///home/user/qt/qtbase/qml/QtQuick/Controls/Imagine/:/images/applicationwindow-background")
    //
    // To ensure that the correct URL is constructed, we do it ourselves here,
    // and then the control QML files use the "url" property instead.
    const QString path = ensureSlash(m_path);
    if (path.startsWith(QLatin1String("qrc")))
        return QUrl(path);

    if (path.startsWith(QLatin1String(":/")))
        return QUrl(QLatin1String("qrc") + path);

    return QUrl::fromLocalFile(path);
}

void QQuickImagineStyle::attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent)
{
    Q_UNUSED(oldParent);
    QQuickImagineStyle *imagine = qobject_cast<QQuickImagineStyle *>(newParent);
    if (imagine)
        inheritPath(imagine->path());
}

static QByteArray resolveSetting(const QByteArray &env, const QSharedPointer<QSettings> &settings, const QString &name)
{
    QByteArray value = qgetenv(env);
#if QT_CONFIG(settings)
    if (value.isNull() && !settings.isNull())
        value = settings->value(name).toByteArray();
#endif
    return value;
}

void QQuickImagineStyle::init()
{
    static bool globalsInitialized = false;
    if (!globalsInitialized) {
        QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(QStringLiteral("Imagine"));

        QString path = QString::fromUtf8(resolveSetting("QT_QUICK_CONTROLS_IMAGINE_PATH", settings, QStringLiteral("Path")));
        if (!path.isEmpty())
            *GlobalPath() = m_path = ensureSlash(path);

        globalsInitialized = true;
    }

    QQuickAttachedPropertyPropagator::initialize(); // TODO: lazy init?
}

QT_END_NAMESPACE

#include "moc_qquickimaginestyle_p.cpp"
