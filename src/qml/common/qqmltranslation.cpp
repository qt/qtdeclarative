// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qqmltranslation_p.h"

QQmlTranslation::QQmlTranslation(const Data &d) : data(d) { }
QQmlTranslation::QQmlTranslation() : data(nullptr) { }

QString QQmlTranslation::translate() const
{
    return std::visit(
            [](auto &&arg) -> QString {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (!std::is_same_v<T, std::nullptr_t>)
                    return arg.translate();
                else {
                    Q_ASSERT_X(false, "QQmlTranslation", "Uninitialized Translation");
                    return {};
                }
            },
            data);
}

QString QQmlTranslation::serializeForQmltc() const
{
    return std::visit(
            [](auto &&arg) -> QString {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (!std::is_same_v<T, std::nullptr_t>)
                    return arg.serializeForQmltc();
                else {
                    Q_ASSERT_X(false, "QQmlTranslation", "Uninitialized Translation");
                    return {};
                }
            },
            data);
}

QString QQmlTranslation::idForQmlDebug() const
{
    return std::visit(
            [](auto &&arg) -> QString {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (!std::is_same_v<T, std::nullptr_t>)
                    return arg.idForQmlDebug();
                else {
                    Q_ASSERT_X(false, "QQmlTranslation", "Uninitialized Translation");
                    return {};
                }
            },
            data);
}

QQmlTranslation::QsTrData::QsTrData(const QString &context, const QString &text,
                                    const QString &comment, int number)
    : context(context.toUtf8()), text(text.toUtf8()), comment(comment.toUtf8()), number(number)
{
}

QString QQmlTranslation::contextFromQmlFilename(const QString &qmlFilename)
{
    int lastSlash = qmlFilename.lastIndexOf(QLatin1Char('/'));
    QStringView contextView = (lastSlash > -1)
            ? QStringView{ qmlFilename }.mid(lastSlash + 1, qmlFilename.size() - lastSlash - 5)
            : QStringView();
    return contextView.toString();
}

QString QQmlTranslation::QsTrData::translate() const
{
#if !QT_CONFIG(translation)
    return QString();
#else
    return QCoreApplication::translate(context, text, comment, number);
#endif
}

QString QQmlTranslation::QsTrData::idForQmlDebug() const
{
    return QString::fromUtf8(text);
}

QString QQmlTranslation::QsTrData::serializeForQmltc() const
{
    QString result = QStringLiteral(R"(QQmlTranslation(QQmlTranslation::QsTrData(
    QStringLiteral("%1"),
    QStringLiteral("%2"),
    QStringLiteral("%3"),
    %4)))")
                             .arg(QString::fromUtf8(context), QString::fromUtf8(text),
                                  QString::fromUtf8(comment))
                             .arg(number);

    return result;
}

QQmlTranslation::QsTrIdData::QsTrIdData(const QString &id, int number)
    : id(id.toUtf8()), number(number)
{
}

QString QQmlTranslation::QsTrIdData::translate() const
{
#if !QT_CONFIG(translation)
    return QString();
#else
    return qtTrId(id, number);
#endif
}

QString QQmlTranslation::QsTrIdData::serializeForQmltc() const
{
    QString result = QStringLiteral(R"(QQmlTranslation(QQmlTranslation::QsTrIdData(
    QStringLiteral("%1"),
    %4)))")
                             .arg(QString::fromUtf8(id))
                             .arg(number);

    return result;
}

QString QQmlTranslation::QsTrIdData::idForQmlDebug() const
{
    return QString::fromUtf8(id);
}
