// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfilenamefilter_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFileNameFilter, "qt.quick.dialogs.qquickfilenamefilter")

QQuickFileNameFilter::QQuickFileNameFilter(QObject *parent)
    : QObject(parent), m_index(-1)
{
}

int QQuickFileNameFilter::index() const
{
    return m_index;
}

void QQuickFileNameFilter::setIndex(int index)
{
    if (m_index == index)
        return;

    m_index = index;
    emit indexChanged(index);
}

QString QQuickFileNameFilter::name() const
{
    return m_name;
}

QStringList QQuickFileNameFilter::extensions() const
{
    return m_extensions;
}

QStringList QQuickFileNameFilter::globs() const
{
    return m_globs;
}

QSharedPointer<QFileDialogOptions> QQuickFileNameFilter::options() const
{
    return m_options;
}

void QQuickFileNameFilter::setOptions(const QSharedPointer<QFileDialogOptions> &options)
{
    m_options = options;
}

static QString extractName(const QString &filter)
{
    return filter.left(filter.indexOf(QLatin1Char('(')) - 1);
}

static QString extractExtension(QStringView filter)
{
    return filter.mid(filter.indexOf(QLatin1Char('.')) + 1).toString();
}

static void extractExtensionsAndGlobs(QStringView filter, QStringList &extensions, QStringList &globs)
{
    extensions.clear();
    globs.clear();

    const int from = filter.indexOf(QLatin1Char('('));
    const int to = filter.lastIndexOf(QLatin1Char(')')) - 1;
    if (from >= 0 && from < to) {
        const QStringView ref = filter.mid(from + 1, to - from);
        const QList<QStringView> exts = ref.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        // For example, given the filter "HTML files (*.html *.htm)",
        // "ref" would be "*.html" and "*.htm".
        for (const QStringView &ref : exts) {
            extensions += extractExtension(ref);
            globs += ref.toString();
        }
    }
}

void QQuickFileNameFilter::update(const QString &filter)
{
    const QStringList filters = nameFilters();

    const int oldIndex = m_index;
    const QString oldName = m_name;
    const QStringList oldExtensions = m_extensions;
    const QStringList oldGlobs = m_globs;

    m_index = filters.indexOf(filter);
    m_name = extractName(filter);
    extractExtensionsAndGlobs(filter, m_extensions, m_globs);

    if (oldIndex != m_index)
        emit indexChanged(m_index);
    if (oldName != m_name)
        emit nameChanged(m_name);
    if (oldExtensions != m_extensions)
        emit extensionsChanged(m_extensions);
    if (oldGlobs != m_globs)
        emit globsChanged(m_globs);

    qCDebug(lcFileNameFilter).nospace() << "update called on " << this << " of " << parent()
        << " with filter " << filter << " (current filters are " << filters << "):"
        << "\n    old index=" << oldIndex << "new index=" << m_index
        << "\n    old name=" << oldName << "new name=" << m_name
        << "\n    old extensions=" << oldExtensions << "new extensions=" << m_extensions
        << "\n    old glob=s" << oldGlobs << "new globs=" << m_globs;
}

QStringList QQuickFileNameFilter::nameFilters() const
{
    return m_options ? m_options->nameFilters() : QStringList();
}

QString QQuickFileNameFilter::nameFilter(int index) const
{
    return m_options ? m_options->nameFilters().value(index) : QString();
}

QT_END_NAMESPACE

#include "moc_qquickfilenamefilter_p.cpp"
