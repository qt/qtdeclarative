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

#include "qv4regexp_p.h"
#include "qv4engine_p.h"
#include "qv4scopedvalue_p.h"
#include <private/qv4mm_p.h>

using namespace QV4;

RegExpCache::~RegExpCache()
{
    for (RegExpCache::Iterator it = begin(), e = end(); it != e; ++it) {
        if (RegExp *re = it.value().as<RegExp>())
            re->d()->cache = nullptr;
    }
}

DEFINE_MANAGED_VTABLE(RegExp);

uint RegExp::match(const QString &string, int start, uint *matchOffsets)
{
    if (!isValid())
        return JSC::Yarr::offsetNoMatch;

    WTF::String s(string);

#if ENABLE(YARR_JIT)
    if (d()->hasValidJITCode())
        return uint(jitCode()->execute(s.characters16(), start, s.length(), (int*)matchOffsets).start);
#endif

    return JSC::Yarr::interpret(byteCode(), s.characters16(), string.length(), start, matchOffsets);
}

Heap::RegExp *RegExp::create(ExecutionEngine* engine, const QString& pattern, bool ignoreCase, bool multiline, bool global)
{
    RegExpCacheKey key(pattern, ignoreCase, multiline, global);

    RegExpCache *cache = engine->regExpCache;
    if (!cache)
        cache = engine->regExpCache = new RegExpCache;

    QV4::WeakValue &cachedValue = (*cache)[key];
    if (QV4::RegExp *result = cachedValue.as<RegExp>())
        return result->d();

    Scope scope(engine);
    Scoped<RegExp> result(scope, engine->memoryManager->alloc<RegExp>(engine, pattern, ignoreCase, multiline, global));

    result->d()->cache = cache;
    cachedValue.set(engine, result);

    return result->d();
}

void Heap::RegExp::init(ExecutionEngine *engine, const QString &pattern, bool ignoreCase, bool multiline, bool global)
{
    Base::init();
    this->pattern = new QString(pattern);
    this->ignoreCase = ignoreCase;
    this->multiLine = multiline;
    this->global = global;

    valid = false;

    const char* error = nullptr;
    JSC::Yarr::YarrPattern yarrPattern(WTF::String(pattern), ignoreCase, multiLine, &error);
    if (error)
        return;
    subPatternCount = yarrPattern.m_numSubpatterns;
#if ENABLE(YARR_JIT)
    if (!yarrPattern.m_containsBackreferences && engine->canJIT()) {
        jitCode = new JSC::Yarr::YarrCodeBlock;
        JSC::JSGlobalData dummy(internalClass->engine->regExpAllocator);
        JSC::Yarr::jitCompile(yarrPattern, JSC::Yarr::Char16, &dummy, *jitCode);
    }
#else
    Q_UNUSED(engine)
#endif
    if (hasValidJITCode()) {
        valid = true;
        return;
    }
    OwnPtr<JSC::Yarr::BytecodePattern> p = JSC::Yarr::byteCompile(yarrPattern, internalClass->engine->bumperPointerAllocator);
    byteCode = p.take();
    if (byteCode)
        valid = true;
}

void Heap::RegExp::destroy()
{
    if (cache) {
        RegExpCacheKey key(this);
        cache->remove(key);
    }
#if ENABLE(YARR_JIT)
    delete jitCode;
#endif
    delete byteCode;
    delete pattern;
    Base::destroy();
}
