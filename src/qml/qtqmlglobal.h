/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
**
****************************************************************************/

#ifndef QTQMLGLOBAL_H
#define QTQMLGLOBAL_H

#if defined(QT_BUILD_QMLDEVTOOLS_LIB) || defined(QT_QMLDEVTOOLS_LIB)
#  define QT_QML_BOOTSTRAPPED
#endif

#include <QtCore/qglobal.h>
#ifndef QT_QML_BOOTSTRAPPED
#  include <QtQml/qtqml-config.h>
#  if QT_CONFIG(qml_network)
#    include <QtNetwork/qtnetworkglobal.h>
#  endif
#else
#  define QT_FEATURE_qml_debug -1
#  define QT_FEATURE_qml_sequence_object 1
#  define QT_FEATURE_qml_jit -1
#  define QT_FEATURE_qml_worker_script -1
#  define QT_FEATURE_qml_xml_http_request -1
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_QML_BOOTSTRAPPED) && !defined(QT_STATIC)
#  if defined(QT_BUILD_QML_LIB)
#    define Q_QML_EXPORT Q_DECL_EXPORT
#  else
#    define Q_QML_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_QML_EXPORT
#endif

QT_END_NAMESPACE
#endif // QTQMLGLOBAL_H
