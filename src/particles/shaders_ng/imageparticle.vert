#version 440

layout(location = 1) in vec4 vData; // x = time, y = lifeSpan, z = size, w = endSize
layout(location = 2) in vec4 vVec;  // x,y = constant velocity,  z,w = acceleration

#if defined(DEFORM)
layout(location = 0) in vec4 vPosTex;
#else
layout(location = 0) in vec2 vPos;
#endif

#if defined(COLOR)
layout(location = 3) in vec4 vColor;
#endif

#if defined(DEFORM)
layout(location = 4) in vec4 vDeformVec; // x,y x unit vector; z,w = y unit vector
layout(location = 5) in vec3 vRotation;  // x = radians of rotation, y = rotation velocity, z = bool autoRotate
#endif

#if defined(SPRITE)
layout(location = 6) in vec3 vAnimData; // w,h(premultiplied of anim), interpolation progress
layout(location = 7) in vec4 vAnimPos;  // x,y, x,y (two frames for interpolation)
#endif

#if defined(TABLE)
layout(location = 0) out vec2 tt; //y is progress if Sprite mode
#endif

#if defined(SPRITE)
layout(location = 1) out vec4 fTexS;
#elif defined(DEFORM)
layout(location = 1) out vec2 fTex;
#endif

#if defined(COLOR)
layout(location = 2) out vec4 fColor;
#else
layout(location = 2) out float fFade;
#endif

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
    float entry;
    float timestamp;
    float sizetable[64];
    float opacitytable[64];
} ubuf;

out gl_PerVertex { vec4 gl_Position; float gl_PointSize; };

void main()
{
    float t = (ubuf.timestamp - vData.x) / vData.y;
    if (t < 0. || t > 1.) {
#if defined(DEFORM)
        gl_Position = ubuf.matrix * vec4(vPosTex.x, vPosTex.y, 0., 1.);
#else
        gl_PointSize = 0.;
#endif
    } else {
#if defined(SPRITE)
        tt.y = vAnimData.z;

        // Calculate frame location in texture
        fTexS.xy = vAnimPos.xy + vPosTex.zw * vAnimData.xy;

        // Next frame is also passed, for interpolation
        fTexS.zw = vAnimPos.zw + vPosTex.zw * vAnimData.xy;

#elif defined(DEFORM)
        fTex = vPosTex.zw;
#endif
        float currentSize = mix(vData.z, vData.w, t * t);
        float fade = 1.;
        float fadeIn = min(t * 10., 1.);
        float fadeOut = 1. - clamp((t - 0.75) * 4.,0., 1.);

#if defined(TABLE)
        currentSize = currentSize * ubuf.sizetable[int(floor(t*64.))];
        fade = fade * ubuf.opacitytable[int(floor(t*64.))];
#endif

        if (ubuf.entry == 1.)
            fade = fade * fadeIn * fadeOut;
        else if (ubuf.entry == 2.)
            currentSize = currentSize * fadeIn * fadeOut;

        if (currentSize <= 0.) {
#if defined(DEFORM)
            gl_Position = ubuf.matrix * vec4(vPosTex.x, vPosTex.y, 0., 1.);
#else
            gl_PointSize = 0.;
#endif
        } else {
            if (currentSize < 3.) // Sizes too small look jittery as they move
                currentSize = 3.;

            vec2 pos;
#if defined(DEFORM)
            float rotation = vRotation.x + vRotation.y * t * vData.y;
            if (vRotation.z == 1.0) {
                vec2 curVel = vVec.zw * t * vData.y + vVec.xy;
                if (length(curVel) > 0.)
                    rotation += atan(curVel.y, curVel.x);
            }
            vec2 trigCalcs = vec2(cos(rotation), sin(rotation));
            vec4 deform = vDeformVec * currentSize * (vPosTex.zzww - 0.5);
            vec4 rotatedDeform = deform.xxzz * trigCalcs.xyxy;
            rotatedDeform = rotatedDeform + (deform.yyww * trigCalcs.yxyx * vec4(-1.,1.,-1.,1.));
            /* The readable version:
            vec2 xDeform = vDeformVec.xy * currentSize * (vTex.x-0.5);
            vec2 yDeform = vDeformVec.zw * currentSize * (vTex.y-0.5);
            vec2 xRotatedDeform;
            xRotatedDeform.x = trigCalcs.x*xDeform.x - trigCalcs.y*xDeform.y;
            xRotatedDeform.y = trigCalcs.y*xDeform.x + trigCalcs.x*xDeform.y;
            vec2 yRotatedDeform;
            yRotatedDeform.x = trigCalcs.x*yDeform.x - trigCalcs.y*yDeform.y;
            yRotatedDeform.y = trigCalcs.y*yDeform.x + trigCalcs.x*yDeform.y;
            */
            pos = vPosTex.xy
                  + rotatedDeform.xy
                  + rotatedDeform.zw
                  + vVec.xy * t * vData.y         // apply velocity
                  + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration
#else
            pos = vPos
                  + vVec.xy * t * vData.y         // apply velocity vector..
                  + 0.5 * vVec.zw * pow(t * vData.y, 2.);
            gl_PointSize = currentSize;
#endif
            gl_Position = ubuf.matrix * vec4(pos.x, pos.y, 0, 1);

#if defined(COLOR)
            fColor = vColor * fade;
#else
            fFade = fade;
#endif
#if defined(TABLE)
            tt.x = t;
#endif
        }
    }
}
