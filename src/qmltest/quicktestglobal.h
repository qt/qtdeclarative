// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TESTQUICKGLOBAL_H
#define TESTQUICKGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#  if defined(QT_BUILD_QMLTEST_LIB)
#    define Q_QUICK_TEST_EXPORT Q_DECL_EXPORT
#  else
#    define Q_QUICK_TEST_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_QUICK_TEST_EXPORT
#endif

QT_END_NAMESPACE

#endif
