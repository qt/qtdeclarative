attribute highp vec2 vPos;
attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize
attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration
uniform highp float entry;
#ifdef COLOR
attribute lowp vec4 vColor;
#endif
#ifdef DEFORM
attribute highp vec2 vTex;
attribute highp vec4 vDeformVec; //x,y x unit vector; z,w = y unit vector
attribute highp vec3 vRotation; //x = radians of rotation, y=rotation speed, z= bool autoRotate
#endif
#ifdef SPRITE
attribute highp vec4 vAnimData;// interpolate(bool), duration, frameCount (this anim), timestamp (this anim)
attribute highp vec4 vAnimPos;//sheet x,y, width/height of this anim
uniform highp vec2 animSheetSize; //width/height of whole sheet
#endif

uniform highp mat4 qt_Matrix;
uniform highp float timestamp;
#ifdef TABLE
varying lowp vec2 tt;//y is progress if Sprite mode
uniform highp float sizetable[64];
uniform highp float opacitytable[64];
#endif
#ifdef SPRITE
varying highp vec4 fTexS;
#else
#ifdef DEFORM
varying highp vec2 fTex;
#endif
#endif
#ifdef COLOR
varying lowp vec4 fColor;
#else
varying lowp float fFade;
#endif


void main() {

    highp float t = (timestamp - vData.x) / vData.y;
    if (t < 0. || t > 1.){
#ifdef DEFORM //Not point sprites
        gl_Position = qt_Matrix * vec4(vPos.x, vPos.y, 0., 1.);
#else
        gl_PointSize = 0.;
#endif
        return;
    }
#ifdef SPRITE
    //Calculate frame location in texture
    highp float frameIndex = mod((((timestamp - vAnimData.w)*1000.)/vAnimData.y),vAnimData.z);
    tt.y = mod((timestamp - vAnimData.w)*1000., vAnimData.y) / vAnimData.y;

    frameIndex = floor(frameIndex);
    fTexS.xy = vec2(((frameIndex + vTex.x) * vAnimPos.z / animSheetSize.x), ((vAnimPos.y + vTex.y * vAnimPos.w) / animSheetSize.y));

    //Next frame is also passed, for interpolation
    //### Should the next anim be precalculated to allow for interpolation there?
    if(vAnimData.x == 1.0 && frameIndex != vAnimData.z - 1.)//Can't do it for the last frame though, this anim may not loop
        frameIndex = mod(frameIndex+1., vAnimData.z);
    fTexS.zw = vec2(((frameIndex + vTex.x) * vAnimPos.z / animSheetSize.x), ((vAnimPos.y + vTex.y * vAnimPos.w) / animSheetSize.y));
#else
#ifdef DEFORM
    fTex = vTex;
#endif
#endif
    highp float currentSize = mix(vData.z, vData.w, t * t);
    lowp float fade = 1.;
    highp float fadeIn = min(t * 10., 1.);
    highp float fadeOut = 1. - clamp((t - 0.75) * 4.,0., 1.);

#ifdef TABLE
    currentSize = currentSize * sizetable[int(floor(t*64.))];
    fade = fade * opacitytable[int(floor(t*64.))];
#endif

    if (entry == 1.)
        fade = fade * fadeIn * fadeOut;
    else if(entry == 2.)
        currentSize = currentSize * fadeIn * fadeOut;

    if(currentSize <= 0)//Sizes too small look jittery as they move
        currentSize = 0;
    else if(currentSize < 3)
        currentSize = 3;

    highp vec2 pos;
#ifdef DEFORM
    highp float rotation = vRotation.x + vRotation.y * t * vData.y;
    if(vRotation.z == 1.0){
        highp vec2 curVel = vVec.zw * t * vData.y + vVec.xy;
        rotation += atan(curVel.y, curVel.x);
    }
    highp vec2 trigCalcs = vec2(cos(rotation), sin(rotation));
    highp vec4 deform = vDeformVec * currentSize * (vTex.xxyy - 0.5);
    highp vec4 rotatedDeform = deform.xxzz * trigCalcs.xyxy;
    rotatedDeform = rotatedDeform + (deform.yyww * trigCalcs.yxyx * vec4(-1.,1.,-1.,1.));
    /* The readable version:
    highp vec2 xDeform = vDeformVec.xy * currentSize * (vTex.x-0.5);
    highp vec2 yDeform = vDeformVec.zw * currentSize * (vTex.y-0.5);
    highp vec2 xRotatedDeform;
    xRotatedDeform.x = trigCalcs.x*xDeform.x - trigCalcs.y*xDeform.y;
    xRotatedDeform.y = trigCalcs.y*xDeform.x + trigCalcs.x*xDeform.y;
    highp vec2 yRotatedDeform;
    yRotatedDeform.x = trigCalcs.x*yDeform.x - trigCalcs.y*yDeform.y;
    yRotatedDeform.y = trigCalcs.y*yDeform.x + trigCalcs.x*yDeform.y;
    */
    pos = vPos
          + rotatedDeform.xy
          + rotatedDeform.zw
          + vVec.xy * t * vData.y         // apply speed
          + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration
#else
    pos = vPos
          + vVec.xy * t * vData.y         // apply speed vector..
          + 0.5 * vVec.zw * pow(t * vData.y, 2.);
    gl_PointSize = currentSize;
#endif
    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);

#ifdef COLOR
    fColor = vColor * fade;
#else
    fFade = fade;
#endif
#ifdef TABLE
    tt.x = t;
#endif
}
