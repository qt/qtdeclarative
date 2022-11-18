// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimageselector_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcache.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qfileselector.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/private/qqmlproperty_p.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQtQuickControlsImageSelector, "qt.quick.controls.imageselector")

static const int DEFAULT_CACHE = 500;

static inline int cacheSize()
{
    static bool ok = false;
    static const int size = qEnvironmentVariableIntValue("QT_QUICK_CONTROLS_IMAGESELECTOR_CACHE", &ok);
    return ok ? size : DEFAULT_CACHE;
}

// input: [focused, pressed]
// => [[focused, pressed], [pressed, focused], [focused], [pressed]]
static QList<QStringList> permutations(const QStringList &input, int count = -1)
{
    if (count == -1)
        count = input.size();

    QList<QStringList> output;
    for (int i = 0; i < input.size(); ++i) {
        QStringList sub = input.mid(i, count);

        if (count > 1) {
            if (i + count > input.size())
                sub += input.mid(0, count - i + 1);

            std::sort(sub.begin(), sub.end());
            do {
                if (!sub.isEmpty())
                    output += sub;
            } while (std::next_permutation(sub.begin(), sub.end()));
        } else {
            output += sub;
        }

        if (count == input.size())
            break;
    }

    if (count > 1)
        output += permutations(input, --count);

    return output;
}

static QString findFile(const QDir &dir, const QString &baseName, const QStringList &extensions)
{
    for (const QString &ext : extensions) {
        QString filePath = dir.filePath(baseName + QLatin1Char('.') + ext);
        if (QFile::exists(filePath))
            return QFileSelector().select(filePath);
    }
    // return an empty string to indicate that the lookup has been done
    // even if no matching asset was found
    return QLatin1String("");
}

QQuickImageSelector::QQuickImageSelector(QObject *parent)
    : QObject(parent),
      m_cache(cacheSize() > 0)
{
}

QUrl QQuickImageSelector::source() const
{
    return m_source;
}

void QQuickImageSelector::setSource(const QUrl &source)
{
    if (m_property.isValid())
        QQmlPropertyPrivate::write(m_property, source, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
    if (m_source == source)
        return;

    m_source = source;
    emit sourceChanged();
}

QString QQuickImageSelector::name() const
{
    return m_name;
}

void QQuickImageSelector::setName(const QString &name)
{
    if (m_name == name)
        return;

    m_name = name;
    if (m_complete)
        updateSource();
}

QString QQuickImageSelector::path() const
{
    return m_path;
}

void QQuickImageSelector::setPath(const QString &path)
{
    if (m_path == path)
        return;

    m_path = path;
    if (m_complete)
        updateSource();
}

QVariantList QQuickImageSelector::states() const
{
    return m_allStates;
}

void QQuickImageSelector::setStates(const QVariantList &states)
{
    if (m_allStates == states)
        return;

    m_allStates = states;
    if (updateActiveStates() && m_complete)
        updateSource();
}

QString QQuickImageSelector::separator() const
{
    return m_separator;
}

void QQuickImageSelector::setSeparator(const QString &separator)
{
    if (m_separator == separator)
        return;

    m_separator = separator;
    if (m_complete)
        updateSource();
}

bool QQuickImageSelector::cache() const
{
    return m_cache;
}

void QQuickImageSelector::setCache(bool cache)
{
    m_cache = cache;
}

void QQuickImageSelector::write(const QVariant &value)
{
    setUrl(value.toUrl());
}

void QQuickImageSelector::setTarget(const QQmlProperty &property)
{
    m_property = property;
}

void QQuickImageSelector::classBegin()
{
}

void QQuickImageSelector::componentComplete()
{
    setUrl(m_property.read().toUrl());
    m_complete = true;
    updateSource();
}

QStringList QQuickImageSelector::fileExtensions() const
{
    static const QStringList extensions = QStringList() << QStringLiteral("png");
    return extensions;
}

QString QQuickImageSelector::cacheKey() const
{
    if (!m_cache)
        return QString();

    return m_path + m_name + m_activeStates.join(m_separator);
}

void QQuickImageSelector::updateSource()
{
    static QCache<QString, QString> cache(cacheSize());

    const QString key = cacheKey();

    QString bestFilePath;

    if (m_cache) {
        QString *cachedPath = cache.object(key);
        if (cachedPath)
            bestFilePath = *cachedPath;
    }

    // note: a cached file path may be empty
    if (bestFilePath.isNull()) {
        QDir dir(m_path);
        int bestScore = -1;

        const QStringList extensions = fileExtensions();

        const QList<QStringList> statePerms = permutations(m_activeStates);
        for (const QStringList &perm : statePerms) {
            const QString filePath = findFile(dir, m_name + m_separator + perm.join(m_separator), extensions);
            if (!filePath.isEmpty()) {
                int score = calculateScore(perm);
                if (score > bestScore) {
                    bestScore = score;
                    bestFilePath = filePath;
                }
            }
        }

        if (bestFilePath.isEmpty())
            bestFilePath = findFile(dir, m_name, extensions);

        if (m_cache)
            cache.insert(key, new QString(bestFilePath));
    }

    qCDebug(lcQtQuickControlsImageSelector) << m_name << m_activeStates << "->" << bestFilePath;

    if (bestFilePath.startsWith(QLatin1Char(':')))
        setSource(QUrl(QLatin1String("qrc") + bestFilePath));
    else
        setSource(QUrl::fromLocalFile(bestFilePath));
}

void QQuickImageSelector::setUrl(const QUrl &url)
{
    QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(url));
    setName(fileInfo.fileName());
    setPath(fileInfo.path());
}

bool QQuickImageSelector::updateActiveStates()
{
    QStringList active;
    for (const QVariant &v : std::as_const(m_allStates)) {
        const QVariantMap state = v.toMap();
        if (state.isEmpty())
            continue;
        auto it = state.begin();
        if (it.value().toBool())
            active += it.key();
    }

    if (m_activeStates == active)
        return false;

    m_activeStates = active;
    return true;
}

int QQuickImageSelector::calculateScore(const QStringList &states) const
{
    int score = 0;
    for (int i = 0; i < states.size(); ++i)
        score += (m_activeStates.size() - m_activeStates.indexOf(states.at(i))) << 1;
    return score;
}

QQuickNinePatchImageSelector::QQuickNinePatchImageSelector(QObject *parent)
    : QQuickImageSelector(parent)
{
}

QStringList QQuickNinePatchImageSelector::fileExtensions() const
{
    static const QStringList extensions = QStringList() << QStringLiteral("9.png") << QStringLiteral("png");
    return extensions;
}

QQuickAnimatedImageSelector::QQuickAnimatedImageSelector(QObject *parent)
    : QQuickImageSelector(parent)
{
}

QStringList QQuickAnimatedImageSelector::fileExtensions() const
{
    static const QStringList extensions = QStringList() << QStringLiteral("webp") << QStringLiteral("gif");
    return extensions;
}

QT_END_NAMESPACE

#include "moc_qquickimageselector_p.cpp"
