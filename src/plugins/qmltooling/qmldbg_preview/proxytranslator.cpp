/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "proxytranslator.h"

#include <QtCore/qlibraryinfo.h>

QT_BEGIN_NAMESPACE

void ProxyTranslator::addEngine(QQmlEngine *engine)
{
    m_engines.append(engine);
}

void ProxyTranslator::removeEngine(QQmlEngine *engine)
{
    m_engines.removeOne(engine);
}

bool ProxyTranslator::hasTranslation(const TranslationBindingInformation &translationBindingInformation) const
{
    resetTranslationFound();
    translationFromInformation(translationBindingInformation);
    return translationFound();
}

QString ProxyTranslator::translationFromInformation(const TranslationBindingInformation &translationBindingInformation)
{
    return translationBindingInformation.compilationUnit->bindingValueAsString(
        translationBindingInformation.compiledBinding);
}


QString ProxyTranslator::originStringFromInformation(const TranslationBindingInformation &translationBindingInformation)
{
    return translationBindingInformation.compilationUnit->stringAt(
        translationBindingInformation.compiledBinding->stringIndex);
}

QQmlSourceLocation ProxyTranslator::sourceLocationFromInformation(const TranslationBindingInformation &translationBindingInformation)
{
    return QQmlSourceLocation(translationBindingInformation.compilationUnit->fileName(),
                              translationBindingInformation.compiledBinding->valueLocation.line,
                              translationBindingInformation.compiledBinding->valueLocation.column);
}


void ProxyTranslator::setLanguage(const QUrl &context, const QLocale &locale)
{
    m_enable = true;
    m_currentUILanguages = locale.uiLanguages().join(QLatin1Char(' '));

    m_qtTranslator.reset(new QTranslator());
    if (!m_qtTranslator->load(locale, QLatin1String("qt"), QLatin1String("_"),
                              QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        m_qtTranslator.reset();
    }

    m_qmlTranslator.reset(new QTranslator(this));
    if (!m_qmlTranslator->load(locale, QLatin1String("qml"), QLatin1String("_"),
                              context.toLocalFile() + QLatin1String("/i18n"))) {
        m_qmlTranslator.reset();
    }

    // unfortunately setUiLanguage set new translators, so do this first
    for (QQmlEngine *engine : qAsConst(m_engines))
        engine->setUiLanguage(locale.bcp47Name());

    // make sure proxy translator is the first used translator
    QCoreApplication::removeTranslator(this);
    QCoreApplication::installTranslator(this);

    for (QQmlEngine *engine : qAsConst(m_engines)) {
        // have two retranslate runs to get elided warning even the same language was set
        m_enable = false;
        engine->retranslate();
        m_enable = true;
        engine->retranslate();
    }
    emit languageChanged(locale);
}

QString ProxyTranslator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    if (!m_enable)
        return {};
    QString result;
    if (result.isNull() && m_qtTranslator)
        result = m_qtTranslator->translate(context, sourceText, disambiguation, n);
    if (result.isNull() && m_qmlTranslator)
        result = m_qmlTranslator->translate(context, sourceText, disambiguation, n);
    m_translationFound = !(result.isNull() || result.isEmpty() || result == sourceText);
    return result;
}

void ProxyTranslator::resetTranslationFound() const
{
    m_translationFound = false;
}

bool ProxyTranslator::translationFound() const
{
    return m_translationFound;
}

bool ProxyTranslator::isEmpty() const
{
    if (m_qtTranslator && m_qtTranslator->isEmpty())
        return false;
    if (m_qmlTranslator && m_qmlTranslator->isEmpty())
        return false;
    return true;
}

QString ProxyTranslator::currentUILanguages() const
{
    return m_currentUILanguages;
}

QT_END_NAMESPACE
