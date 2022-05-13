// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4_ALLOCA_H
#define QV4_ALLOCA_H

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

#include <QtCore/private/qglobal_p.h>

#if QT_CONFIG(alloca_h)
#  include <alloca.h>
#elif QT_CONFIG(alloca_malloc_h)
#  include <malloc.h>
// This does not matter unless compiling in strict standard mode.
#  ifdef Q_CC_MSVC
#    define alloca _alloca
#  endif
#else
#  include <stdlib.h>
#endif

// Define Q_ALLOCA_VAR macro to be used instead of #ifdeffing
// the occurrences of alloca() in case it's not supported.
// Q_ALLOCA_DECLARE and Q_ALLOCA_ASSIGN macros separate
// memory allocation from the declaration and RAII.
#define Q_ALLOCA_VAR(type, name, size) \
    Q_ALLOCA_DECLARE(type, name); \
    Q_ALLOCA_ASSIGN(type, name, size)

#if QT_CONFIG(alloca)

#define Q_ALLOCA_DECLARE(type, name) \
    type *name = 0

#define Q_ALLOCA_ASSIGN(type, name, size) \
    name = static_cast<type*>(alloca(size))

#else
QT_BEGIN_NAMESPACE
class Qt_AllocaWrapper
{
public:
    Qt_AllocaWrapper() { m_data = 0; }
    ~Qt_AllocaWrapper() { free(m_data); }
    void *data() { return m_data; }
    void allocate(int size) { m_data = malloc(size); memset(m_data, 0, size); }
private:
    void *m_data;
};
QT_END_NAMESPACE

#define Q_ALLOCA_DECLARE(type, name) \
    Qt_AllocaWrapper _qt_alloca_##name; \
    type *name = nullptr

#define Q_ALLOCA_ASSIGN(type, name, size) \
    do { \
        _qt_alloca_##name.allocate(size); \
        name = static_cast<type*>(_qt_alloca_##name.data()); \
    } while (false)

#endif

#endif
