attribute highp vec2 vPos;
attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize
attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration

uniform highp mat4 qt_Matrix;
uniform highp float timestamp;
uniform highp float entry;

varying lowp float fFade;

void main() {                                           
    highp float t = (timestamp - vData.x) / vData.y;

    highp float currentSize = mix(vData.z, vData.w, t * t);

    if (t < 0. || t > 1.)
        currentSize = 0.;

    fFade = 1.;

    if (entry == 1.){
        highp float fadeIn = min(t * 10., 1.);
        highp float fadeOut = 1. - max(0., min((t - 0.75) * 4., 1.));
        fFade = fadeIn * fadeOut;
    }else if(entry == 2.){
        highp float sizeIn = min(t * 10., 1.);
        highp float sizeOut = 1. - max(0., min((t - 0.75) * 4., 1.));
        currentSize = currentSize * sizeIn * sizeOut;
    }

    gl_PointSize = currentSize;

    highp vec2 pos = vPos
                   + vVec.xy * t * vData.y         // apply speed vector..
                   + 0.5 * vVec.zw * pow(t * vData.y, 2.);

    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);

}
