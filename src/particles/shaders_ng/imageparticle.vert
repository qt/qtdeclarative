#version 440

layout(location = 1) in vec4 vData; // x = time, y = lifeSpan, z = size, w = endSize
layout(location = 2) in vec4 vVec;  // x,y = constant velocity,  z,w = acceleration

#if defined(DEFORM)
layout(location = 0) in vec4 vPosRot; //x = x, y = y,  z = radians of rotation, w = rotation velocity
#else
layout(location = 0) in vec2 vPos;
#endif

#if defined(COLOR)
layout(location = 3) in vec4 vColor;
#endif

#if !defined(DEFORM) && !defined(POINT) // Color-level
layout(location = 4) in vec2 vTex; // x = tx, y = ty
#endif

#if defined(DEFORM)
layout(location = 4) in vec4 vDeformVec; // x,y x unit vector; z,w = y unit vector
layout(location = 5) in vec3 vTex;  // x = tx, y = ty, z = bool autoRotate
#endif

#if defined(SPRITE)
layout(location = 6) in vec3 vAnimData; // w,h(premultiplied of anim), interpolation progress
layout(location = 7) in vec3 vAnimPos;  // x, y, x2 (two frames for interpolation)
#endif

#if defined(TABLE)
layout(location = 0) out vec2 tt; //y is progress if Sprite mode
#endif

#if defined(SPRITE)
layout(location = 1) out vec4 fTexS;
#elif !defined(POINT)
layout(location = 1) out vec2 fTex;
#endif

#if defined(COLOR)
layout(location = 2) out vec4 fColor;
#else
layout(location = 2) out float fFade;
#endif

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    float opacity;
    float entry;
    float timestamp;
    float dpr;
    float sizetable[64];
    float opacitytable[64];
} ubuf;

void main()
{
    float t = (ubuf.timestamp - vData.x) / vData.y;
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix = ubuf.matrix[gl_ViewIndex];
#else
    mat4 matrix = ubuf.matrix;
#endif

    if (t < 0. || t > 1.) {
#if defined(DEFORM)
        gl_Position = matrix * vec4(vPosRot.x, vPosRot.y, 0., 1.);
#elif defined(POINT)
        gl_PointSize = 0.;
#else
        gl_Position = matrix * vec4(vPos.x, vPos.y, 0., 1.);
#endif
    } else {
#if defined(SPRITE)
        tt.y = vAnimData.z;

        // Calculate frame location in texture
        fTexS.xy = vAnimPos.xy + vTex.xy * vAnimData.xy;

        // Next frame is also passed, for interpolation
        fTexS.zw = vAnimPos.zy + vTex.xy * vAnimData.xy;

#elif !defined(POINT)
        fTex = vTex.xy;
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
            gl_Position = matrix * vec4(vPosRot.x, vPosRot.y, 0., 1.);
#elif defined(POINT)
            gl_PointSize = 0.;
#else
            gl_Position = matrix * vec4(vPos.x, vPos.y, 0., 1.);
#endif

        } else {
            if (currentSize < 3.) // Sizes too small look jittery as they move
                currentSize = 3.;

            vec2 pos;
#if defined(DEFORM)
            float rotation = vPosRot.z + vPosRot.w * t * vData.y;
            if (vTex.z > 0.) {
                vec2 curVel = vVec.zw * t * vData.y + vVec.xy;
                if (length(curVel) > 0.)
                    rotation += atan(curVel.y, curVel.x);
            }
            vec2 trigCalcs = vec2(cos(rotation), sin(rotation));
            vec4 deform = vDeformVec * currentSize * (vTex.xxyy - 0.5);
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
            pos = vPosRot.xy
                  + rotatedDeform.xy
                  + rotatedDeform.zw
                  + vVec.xy * t * vData.y         // apply velocity
                  + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration
#elif defined(POINT)
            pos = vPos.xy
                  + vVec.xy * t * vData.y         // apply velocity vector..
                  + 0.5 * vVec.zw * pow(t * vData.y, 2.);
            gl_PointSize = currentSize * ubuf.dpr;
#else // non point color
            vec2 deform = currentSize * (vTex.xy - 0.5);
            pos = vPos.xy
                  + deform.xy
                  + vVec.xy * t * vData.y         // apply velocity
                  + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration
#endif
            gl_Position = matrix * vec4(pos.x, pos.y, 0, 1);

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
