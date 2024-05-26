// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDTYPES_P_H
#define QANDROIDTYPES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Void, "java/lang/Void");
Q_DECLARE_JNI_CLASS(Integer, "java/lang/Integer");
Q_DECLARE_JNI_CLASS(Double, "java/lang/Double");
Q_DECLARE_JNI_CLASS(Float, "java/lang/Float");
Q_DECLARE_JNI_CLASS(Boolean, "java/lang/Boolean");
Q_DECLARE_JNI_CLASS(String, "java/lang/String");
Q_DECLARE_JNI_CLASS(Class, "java/lang/Class");

QT_END_NAMESPACE

#endif // QANDROIDTYPES_P_H
