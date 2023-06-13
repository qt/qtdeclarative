// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJSLIST_H
#define QJSLIST_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qjsengine.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#include <algorithm>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version. It will be kept compatible with the intended usage by
// code generated using qmlcachegen.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

struct QJSListIndexClamp
{
    static qsizetype clamp(int start, qsizetype max, qsizetype min = 0)
    {
        Q_ASSERT(min >= 0);
        Q_ASSERT(min <= max);
        return std::clamp(start < 0 ? max + qsizetype(start) : qsizetype(start), min, max);
    }
};

template<typename List, typename Value = typename List::value_type>
struct QJSList : private QJSListIndexClamp
{
    Q_DISABLE_COPY_MOVE(QJSList)

    QJSList(List *list, QJSEngine *engine) : m_list(list), m_engine(engine) {}

    bool includes(const Value &value) const
    {
        return std::find(m_list->cbegin(), m_list->cend(), value) != m_list->cend();
    }

    bool includes(const Value &value, int start) const
    {
        return std::find(m_list->cbegin() + clamp(start, m_list->size()), m_list->cend(), value)
                != m_list->cend();
    }

    QString join(const QString &separator = QStringLiteral(",")) const
    {
        QString result;
        bool atBegin = true;
        std::for_each(m_list->cbegin(), m_list->cend(), [&](const Value &value) {
            if (atBegin)
                atBegin = false;
            else
                result += separator;
            result += m_engine->coerceValue<Value, QString>(value);
        });
        return result;
    }

    List slice() const
    {
        return *m_list;
    }
    List slice(int start) const
    {
        List result;
        std::copy(m_list->cbegin() + clamp(start, m_list->size()), m_list->cend(),
                  std::back_inserter(result));
        return result;
    }
    List slice(int start, int end) const
    {
        const qsizetype size = m_list->size();
        const qsizetype clampedStart = clamp(start, size);
        const qsizetype clampedEnd = clamp(end, size, clampedStart);

        List result;
        std::copy(m_list->cbegin() + clampedStart, m_list->cbegin() + clampedEnd,
                  std::back_inserter(result));
        return result;
    }

    int indexOf(const Value &value) const
    {
        const auto begin = m_list->cbegin();
        const auto end = m_list->cend();
        const auto it = std::find(begin, end, value);
        if (it == end)
            return -1;
        const qsizetype result = it - begin;
        Q_ASSERT(result >= 0);
        return result > std::numeric_limits<int>::max() ? -1 : int(result);
    }
    int indexOf(const Value &value, int start) const
    {
        const auto begin = m_list->cbegin();
        const auto end = m_list->cend();
        const auto it = std::find(begin + clamp(start, m_list->size()), end, value);
        if (it == end)
            return -1;
        const qsizetype result = it - begin;
        Q_ASSERT(result >= 0);
        return result > std::numeric_limits<int>::max() ? -1 : int(result);
    }

    int lastIndexOf(const Value &value) const
    {
        const auto begin = std::make_reverse_iterator(m_list->cend());
        const auto end = std::make_reverse_iterator(m_list->cbegin());
        const auto it = std::find(begin, end, value);
        const qsizetype result = (end - it) - 1;
        return result > std::numeric_limits<int>::max() ? -1 : int(result);
    }
    int lastIndexOf(const Value &value, int start) const
    {
        const qsizetype size = m_list->size();
        if (size == 0)
            return -1;

        // Construct a one-past-end iterator as input.
        const qsizetype clampedStart = std::min(clamp(start, size), size - 1);
        const auto begin = std::make_reverse_iterator(m_list->cbegin() + clampedStart + 1);

        const auto end = std::make_reverse_iterator(m_list->cbegin());
        const auto it = std::find(begin, end, value);
        const qsizetype result = (end - it) - 1;
        return result > std::numeric_limits<int>::max() ? -1 : int(result);
    }

    QString toString() const { return join(); }

private:
    List *m_list = nullptr;
    QJSEngine *m_engine = nullptr;
};

template<>
struct QJSList<QQmlListProperty<QObject>, QObject *>  : private QJSListIndexClamp
{
    Q_DISABLE_COPY_MOVE(QJSList)

    QJSList(QQmlListProperty<QObject> *list, QJSEngine *engine) : m_list(list), m_engine(engine) {}

    bool includes(const QObject *value) const
    {
        if (!m_list->count || !m_list->at)
            return false;

        const qsizetype size = m_list->count(m_list);
        for (qsizetype i = 0; i < size; ++i) {
            if (m_list->at(m_list, i) == value)
                return true;
        }

        return false;
    }
    bool includes(const QObject *value, int start) const
    {
        if (!m_list->count || !m_list->at)
            return false;

        const qsizetype size = m_list->count(m_list);
        for (qsizetype i = clamp(start, size); i < size; ++i) {
            if (m_list->at(m_list, i) == value)
                return true;
        }

        return false;
    }

    QString join(const QString &separator = QStringLiteral(",")) const
    {
        if (!m_list->count || !m_list->at)
            return QString();

        QString result;
        for (qsizetype i = 0, end = m_list->count(m_list); i < end; ++i) {
            if (i != 0)
                result += separator;
            result += m_engine->coerceValue<QObject *, QString>(m_list->at(m_list, i));
        }

        return result;
    }

    QObjectList slice() const
    {
        return m_list->toList<QObjectList>();
    }
    QObjectList slice(int start) const
    {
        if (!m_list->count || !m_list->at)
            return QObjectList();

        const qsizetype size = m_list->count(m_list);
        const qsizetype clampedStart = clamp(start, size);
        QObjectList result;
        result.reserve(size - clampedStart);
        for (qsizetype i = clampedStart; i < size; ++i)
            result.append(m_list->at(m_list, i));
        return result;
    }
    QObjectList slice(int start, int end) const
    {
        if (!m_list->count || !m_list->at)
            return QObjectList();

        const qsizetype size = m_list->count(m_list);
        const qsizetype clampedStart = clamp(start, size);
        const qsizetype clampedEnd = clamp(end, size, clampedStart);
        QObjectList result;
        result.reserve(clampedEnd - clampedStart);
        for (qsizetype i = clampedStart; i < clampedEnd; ++i)
            result.append(m_list->at(m_list, i));
        return result;
    }

    int indexOf(const QObject *value) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        const qsizetype end
                = std::min(m_list->count(m_list), qsizetype(std::numeric_limits<int>::max()));
        for (qsizetype i = 0; i < end; ++i) {
            if (m_list->at(m_list, i) == value)
                return int(i);
        }
        return -1;
    }
    int indexOf(const QObject *value, int start) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        const qsizetype size = m_list->count(m_list);
        for (qsizetype i = clamp(start, size),
             end = std::min(size, qsizetype(std::numeric_limits<int>::max()));
             i < end; ++i) {
            if (m_list->at(m_list, i) == value)
                return int(i);
        }
        return -1;
    }

    int lastIndexOf(const QObject *value) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        for (qsizetype i = m_list->count(m_list) - 1; i >= 0; --i) {
            if (m_list->at(m_list, i) == value)
                return i > std::numeric_limits<int>::max() ? -1 : int(i);
        }
        return -1;
    }
    int lastIndexOf(const QObject *value, int start) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        const qsizetype size = m_list->count(m_list);
        if (size == 0)
            return -1;

        qsizetype clampedStart = std::min(clamp(start, size), size - 1);
        for (qsizetype i = clampedStart; i >= 0; --i) {
            if (m_list->at(m_list, i) == value)
                return i > std::numeric_limits<int>::max() ? -1 : int(i);
        }
        return -1;
    }

    QString toString() const { return join(); }

private:
    QQmlListProperty<QObject> *m_list = nullptr;
    QJSEngine *m_engine = nullptr;
};

QT_END_NAMESPACE

#endif // QJSLIST_H
