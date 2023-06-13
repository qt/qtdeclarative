// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmlsasourcelocation.h"
#include "qqmlsasourcelocation_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QQmlSA {

static_assert(SourceLocationPrivate::sizeOfSourceLocation() == sizeof(SourceLocation));

/*!
    \class QQmlSA::SourceLocation
    \inmodule QtQmlCompiler

    \brief Represents a location or region in the source code.
 */
QQmlSA::SourceLocation::SourceLocation(quint32 offset, quint32 length, quint32 line, quint32 column)
{
    new (m_data) QQmlJS::SourceLocation{ offset, length, line, column };
}

// explicitly defaulted out-of-line for PIMPL
QQmlSA::SourceLocation::SourceLocation(const SourceLocation &other) = default;
QQmlSA::SourceLocation & QQmlSA::SourceLocation::operator=(const QQmlSA::SourceLocation &other) = default;
SourceLocation::~SourceLocation() = default;

bool QQmlSA::SourceLocation::isValid() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).isValid();
}

/*!
    Returns the offset of the beginning of this source location.
 */
quint32 QQmlSA::SourceLocation::begin() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).begin();
}

/*!
    Returns the offset of the end of this source location.
 */
quint32 QQmlSA::SourceLocation::end() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).end();
}

/*!
    Returns the offset of the beginning of this source location.
 */
quint32 QQmlSA::SourceLocation::offset() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).offset;
}

/*!
    Returns the length of this source location.
 */
quint32 QQmlSA::SourceLocation::length() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).length;
}

/*!
    Returns the line number containing the beginning of this source location.
 */
quint32 QQmlSA::SourceLocation::startLine() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).startLine;
}

/*!
    Returns the column number containing the beginning of this source location.
 */
quint32 QQmlSA::SourceLocation::startColumn() const
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(*this).startColumn;
}

/*!
    Returns a source location of lenth zero pointing to the beginning of this
    source location.
 */
QQmlSA::SourceLocation QQmlSA::SourceLocation::startZeroLengthLocation() const
{
    QQmlSA::SourceLocation saLocation;
    auto &wrappedLocation = reinterpret_cast<QQmlJS::SourceLocation &>(saLocation.m_data);
    wrappedLocation =
            QQmlSA::SourceLocationPrivate::sourceLocation(*this).startZeroLengthLocation();

    return saLocation;
}

/*!
    Returns a source location of lenth zero pointing to the end of this source
    location pointing to \a text.
 */
QQmlSA::SourceLocation QQmlSA::SourceLocation::endZeroLengthLocation(QStringView text) const
{
    QQmlSA::SourceLocation saLocation;
    auto &wrappedLocation = reinterpret_cast<QQmlJS::SourceLocation &>(saLocation.m_data);
    wrappedLocation = wrappedLocation.endZeroLengthLocation(text);

    return saLocation;
}

qsizetype QQmlSA::SourceLocation::qHashImpl(const SourceLocation &location, qsizetype seed)
{
    return qHash(QQmlSA::SourceLocationPrivate::sourceLocation(location), seed);
}

bool QQmlSA::SourceLocation::operatorEqualsImpl(const SourceLocation &lhs,
                                                const SourceLocation &rhs)
{
    return QQmlSA::SourceLocationPrivate::sourceLocation(lhs)
            == QQmlSA::SourceLocationPrivate::sourceLocation(rhs);
}

} // namespace QQmlSA

QT_END_NAMESPACE
