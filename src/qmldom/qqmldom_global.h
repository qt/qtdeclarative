/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**/
#ifndef QMLDOM_GLOBAL_H
#define QMLDOM_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QMLDOM_DYNAMIC)
#if defined(QMLDOM_LIBRARY)
#  define QMLDOM_EXPORT Q_DECL_EXPORT
#else
#  define QMLDOM_EXPORT Q_DECL_IMPORT
#endif
#else
#  define QMLDOM_EXPORT
#endif

QT_BEGIN_NAMESPACE
// avoid annoying warning about missing QT_BEGIN_NAMESPACE...
QT_END_NAMESPACE

#endif // QMLDOM_GLOBAL_H
