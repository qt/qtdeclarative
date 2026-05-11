// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440
precision highp float;

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;      	// reserved
    float qt_Opacity;	 	// reserved
    vec2 sourceItemSize; 	// The size of the input item. The source is divided into a grid of cells.
    vec4 borderColor;    	// The color of the border (to be masked away). Set to transparent to ignore.
    vec4 particleColor;  	// The color of the noise particle
    float borderMaskEnabled;	// Enabled mask or not. If the bg and border color is the same, set this to 0
    float borderMaskThreshold;	// The threshold for determining if a pixel belongs to the border (taking anti-aliasing into account)
    float particleSize;  	// The size of a dust particle (aka the cell size in the grid)
    float particleOpacity;  	// The particleOpacity of the particle
    float particleDensity;	// The threshold deciding if a particle (aka cell in the grid) should be visible or not
    float time;			// time, for animating the noise
} args;

float random(vec2 st, float t) {
    vec2 offsetSt = st + t;
    return fract(sin(dot(offsetSt.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    vec4 sourceColor = texture(source, qt_TexCoord0);
    vec2 pixelCoord = qt_TexCoord0 * args.sourceItemSize;
    vec2 noiseCoord = floor(pixelCoord / args.particleSize);
    float randomValue = random(noiseCoord, args.time);
    float noiseMix = step(randomValue, args.particleDensity);

    float calculatedBorderMask = step(args.borderMaskThreshold, distance(sourceColor.rgba, args.borderColor.rgba));
    float borderMask = mix(1.0, calculatedBorderMask, args.borderMaskEnabled);
    float finalMask = sourceColor.a * borderMask;
    float finalAlpha = sourceColor.a * args.qt_Opacity;
    float maskedNoiseAlpha = noiseMix * args.particleOpacity * finalMask;

    vec3 blendedColor = mix(sourceColor.rgb, args.particleColor.rgb, maskedNoiseAlpha);
    vec3 preMultipliedColor = blendedColor * args.qt_Opacity;

    fragColor = vec4(preMultipliedColor, finalAlpha);
}
