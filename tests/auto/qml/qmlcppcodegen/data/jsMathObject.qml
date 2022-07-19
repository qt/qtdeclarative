// Copyright (C) 2021 The Qt Company Ltd.

pragma Strict
import QtQml

QtObject {
    property double a: 0
    property double b: 0

    property double abs: Math.abs(a)
    property double acos: Math.acos(a)
    property double acosh: Math.acosh(a)
    property double asin: Math.asin(a)
    property double asinh: Math.asinh(a)
    property double atan: Math.atan(a)
    property double atanh: Math.atanh(a)
//    property double atan2: Math.atan2(a)
    property double cbrt: Math.cbrt(a)
    property double ceil: Math.ceil(a)
    property double clz32: Math.clz32(a)
    property double cos: Math.cos(a)
    property double cosh: Math.cosh(a)
    property double exp: Math.exp(a)
//    property double expm1: Math.expm1(a)
    property double floor: Math.floor(a)
    property double fround: Math.fround(a)
//    property double hypot: Math.hypot(a)
    property double imul: Math.imul(a, b)
    property double log: Math.log(a)
    property double log10: Math.log10(a)
    property double log1p: Math.log1p(a)
    property double log2: Math.log2(a)
    property double max: Math.max(a, b)
    property double min: Math.min(a, b)
    property double pow: Math.pow(a, b)
    property double random: Math.random()
    property double round: Math.round(a)
    property double sign: Math.sign(a)
    property double sin: Math.sin(a)
    property double sinh: Math.sinh(a)
    property double sqrt: Math.sqrt(a)
    property double tan: Math.tan(a)
    property double tanh: Math.tanh(a)
    property double trunc: Math.trunc(a)
}
