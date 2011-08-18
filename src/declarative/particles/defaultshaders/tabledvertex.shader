attribute highp vec2 vPos;
attribute highp vec2 vTex;
attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize
attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration
attribute lowp vec4 vColor;
attribute highp vec4 vDeformVec; //x,y x unit vector; z,w = y unit vector
attribute highp vec3 vRotation; //x = radians of rotation, y=rotation speed, z= bool autoRotate
attribute highp vec4 vAnimData;// idx, duration, frameCount (this anim), timestamp (this anim)

uniform highp mat4 qt_Matrix;
uniform highp float timestamp;

varying lowp float tt;
varying highp vec2 fTex;
varying lowp float progress;
varying lowp vec4 fColor;


void main() {
    highp float size = vData.z;
    highp float endSize = vData.w;

    highp float t = (timestamp - vData.x) / vData.y;

    fTex = vTex;
    highp float currentSize = mix(size, endSize, t * t);
    if (t < 0. || t > 1.)
        currentSize = 0.;

    highp vec2 pos;
    highp float rotation = vRotation.x + vRotation.y * t * vData.y;
    if(vRotation.z == 1.0){
        highp vec2 curVel = vVec.zw * t * vData.y + vVec.xy;
        rotation += atan(curVel.y, curVel.x);
    }
    highp vec2 trigCalcs = vec2(cos(rotation), sin(rotation));
    highp vec2 xDeform = vDeformVec.xy * currentSize * (vTex.x-0.5);
    highp vec2 yDeform = vDeformVec.zw * currentSize * (vTex.y-0.5);
    highp vec2 xRotatedDeform;
    xRotatedDeform.x = trigCalcs.x*xDeform.x - trigCalcs.y*xDeform.y;
    xRotatedDeform.y = trigCalcs.y*xDeform.x + trigCalcs.x*xDeform.y;
    highp vec2 yRotatedDeform;
    yRotatedDeform.x = trigCalcs.x*yDeform.x - trigCalcs.y*yDeform.y;
    yRotatedDeform.y = trigCalcs.y*yDeform.x + trigCalcs.x*yDeform.y;
    pos = vPos
          + xRotatedDeform
          + yRotatedDeform
          //- vec2(1,1) * currentSize * 0.5 // 'center'
          + vVec.xy * t * vData.y         // apply speed
          + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration

    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);

    fColor = vColor;
    tt = t;

}
