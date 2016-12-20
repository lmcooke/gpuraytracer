#version 400 core
#define SPHERE 0
#define CUBE 1
#define CONE 2
#define CYLINDER 3
#define NO_INTERSECT 4
#define LIGHT_POINT 5
#define LIGHT_DIRECTIONAL 6
#define SHAPE_EPSILON .001
#define CONE_SLOPE 2.0
#define MAX_BOUNCE 3
#define PI 3.1415
#define DIFFUSE 7
#define NORMAL 8

// [DATA TYPES]
/////////////////////////////////////////////////////////////////////////

// Scene Object has it's material in the same struct
// Because passing a struct of structs into a shader seems wild
struct SceneObject{
    int primitive; // Can be SPHERE, CUBE, CONE, CYLINDER
    mat4x4 objectToWorld; // cumulative transformation matrix

    // Material properties
    vec4 cDiffuse;
    vec4 cAmbient;
    vec4 cSpecular;
    vec4 cReflective;
    float shininess;

    // Texture properties
    float blend;
    int texID;
    float repeatU;
    float repeatV;
};

// Data structure for intersection test results
struct PrimitiveType{
    float t;
    int primitive; // Can be SPHERE, CUBE, CONE, CYLINDER, NO_INTERSECT
    vec4 cDiffuse;
    vec4 cAmbient;
    vec4 cSpecular;
    vec4 cReflective;
    float shininess;
    mat4x4 objectToWorld;
    float blend;
    int texID;
    float repeatU;
    float repeatV;
};

// Final output of data Raytracing spits out to write to color attachments
struct RayData{
    float rayT;
    int rayObjectID;
    vec4 rayNormal;
    vec4 rayPoint;
    vec4 rayDirection;
};

struct GlobalData{
    float ka; // global ambient coefficient
    float kd; // global diffuse coefficeint
    float ks; // global specular coefficient
    float kt; // global transparency coefficient
};

// Light Data
// Just point lights at the moment
struct LightObject{
    vec4 color;
    vec4 pos; // Only applicable for point lights
    vec4 dir; // Only applicable for directional lights
    vec3 function; // attenuation function
    float lightIntensitySetting;
    int type; // Can be LIGHT_POINT, LIGHT_DIRECTIONAL
};

struct SettingsData{
//    // Lighting intensities
    float l1Intensity;    // The intensity for light 1 (scaled to [0.0, 1.0])
    float l2Intensity;    // The intensity for light 2 (scaled to [0.0, 1.0])
    float l3Intensity;    // The intensity for light 3 (scaled to [0.0, 1.0])

    int useStochastic; // Stochastic Sampling
    int useAO;         // Ambient Occlusion
    int useNM;         // Normal Mapping
    int useDOF;        // Depth of Field
    int aperture;     // Depth of field aperture size
    int focalLength;  // Depth of field focal length
    int numSamples;

    // Lighting equation components
    int useAmbient;
    int useDiffuse;
    int useSpecular;
    int useShadows;
    int useReflections;
    int useEnvironment;
    int useTextures;
};

// [INPUT / OUTPUT]
////////////////////////////////////////////////////////////////////////////
in vec2 uv;
uniform sampler2D prev; // 0
uniform samplerCube envMap;
uniform vec2 dimensions;
uniform mat4x4 inverseCam;
uniform float firstPass;
uniform int numPasses;
uniform float time;

// Structs sent from view.cpp
uniform GlobalData globalData;
uniform SettingsData settings;

// Textures [1 diffuse, 1 normal atm]
uniform sampler2D metalDiffuseTex; // 1
uniform sampler2D metalNormalTex; // 2

uniform sampler2D woodDiffuseTex; // 4
uniform sampler2D woodNormalTex; // 5

uniform sampler2D plasterDiffuseTex; // 6
uniform sampler2D plasterNormalTex; // 7

// Light objects [max 3 atm]
uniform LightObject lightObject1;
uniform LightObject lightObject2;
uniform LightObject lightObject3;

// Scene objects [max 5 atm]
uniform SceneObject sceneObject1;
uniform SceneObject sceneObject2;
uniform SceneObject sceneObject3;
uniform SceneObject sceneObject4;
uniform SceneObject sceneObject5;
uniform SceneObject sceneObject6;

// Output location
out vec4 fragColor;


// [SCENE DATA]
////////////////////////////////////////////////////////////////////////////

// Object list (initialized in main)
SceneObject[6] sceneObjects;

// Light list (initialized in main)
LightObject sceneLights[3];

// [SHAPES]
/////////////////////////////////////////////////////////////////////////

float cap(vec4 objPeye, vec4 objD, vec3 normal, vec3 pt)
{

    float tDenom = (normal.x * objD.x) + (normal.y * objD.y) + (normal.z * objD.z);
    float tNum1 = (normal.x * pt.x) + (normal.y * pt.y) + (normal.z * pt.z);
    float tNum2 = (normal.x * objPeye.x) + (normal.y * objPeye.y) + (normal.z * objPeye.z);
    float t = (tNum1 - tNum2)/tDenom;

    vec4 intersectionPt = objPeye + (t * objD);

    bool withinCap = pow(intersectionPt.x,2.0) + pow(intersectionPt.z, 2.0) <= pow(0.5, 2.0);

    int inCap = int(withinCap);
    int outsideCap = int(!withinCap);
    return (inCap * t) + (outsideCap * -1.0);
}


// finds the intersection 't' of the input ray with a sphere centered at (0,0,0)
// with r=.5
// objPeye and objD must already be in object space
float sphere(vec4 objPeye, vec4 objD)
{

    float a = pow(objD.x, 2.0) + pow(objD.y, 2.0) + pow(objD.z, 2.0);
    float b = (2.0 * objD.x * objPeye.x) + (2.0 * objD.y * objPeye.y) + (2.0 * objD.z * objPeye.z);
    float c = pow(objPeye.x, 2.0) + pow(objPeye.y, 2.0) + pow(objPeye.z, 2.0) - pow(0.5, 2.0);
    float discriminant = pow(b,2.0) - (4.0 * a * c);

    bool intersectCheck = discriminant >= 0;
    int intersection = int(intersectCheck);
    int noIntersection = int(!intersectCheck);

    float t1 = ((-1.0 * b) + sqrt(discriminant))/(2.0 * a);
    float t2 = ((-1.0 * b) - sqrt(discriminant))/(2.0 * a);

    int returnMin = int(t1 > 0 && t2 > 0);
    int returnNone = int(t1 < 0 && t2 < 0);
    int returnMax = int(returnMin == 0 && returnNone == 0);

    float intersectOpt =  (returnMin * min(t1,t2)) +
                            (returnNone * -1.0) +
                            (returnMax * max(t1, t2));

    return (intersection * intersectOpt) +
            (noIntersection * -1.0);
}

// helper method for cylinder
float cylinderBody(vec4 objPeye, vec4 objD)
{
    float a = pow(objD.x, 2.0) + pow(objD.z, 2.0);
    float b = (2.0 * objD.x * objPeye.x) + (2.0 * objD.z * objPeye.z);
    float c = pow(objPeye.x, 2.0) + pow(objPeye.z, 2.0) - pow(0.5, 2.0);
    float discriminant = pow(b,2.0) - (4.0 * a * c);

    bool validDiscriminant = discriminant >= 0;
    int intersectionCheck = int(validDiscriminant);
    int noIntersectionCheck = int(!validDiscriminant);

    float t1 = ((-1.0 * b) + sqrt(discriminant))/(2.0 * a);
    float t2 = ((-1.0 * b) - sqrt(discriminant))/(2.0 * a);

    float t3 = min(t1,t2);
    float y = objPeye.y + (t3 * objD.y);

    bool withinBoundsBool1 = (y >= -.5 && y <= .5);
    int withinBounds1 = int(withinBoundsBool1);
    int notWithinBounds1 = int(!withinBoundsBool1);

    int twoIntersections = int(t1 > 0 && t2 > 0);
    int noIntersections = int(t1 < 0 && t2 < 0);
    int oneIntersection = int(twoIntersections == 0 && noIntersections == 0);

    float intersectedFinalReturn =  (twoIntersections * ((t3 * withinBounds1) + (-1.0 * notWithinBounds1))) +
                                    (noIntersections * -1.0) +
                                    (oneIntersection * ((t3 * withinBounds1) + (-1.0 * notWithinBounds1)));


    return (intersectionCheck * intersectedFinalReturn) +
            (noIntersectionCheck * -1.0);
}

// finds the intersection 't' of the input ray with
// a cylinder centered at (0,0,0) with r=.5 and h=1
// objPeye and objD must already be in object space.
float cylinder(vec4 objPeye, vec4 objD)
{
    float currentT = -1.f;

    float bodyT = cylinderBody(objPeye, objD);

    if (bodyT >= 0) {

        bool validTBool1 = currentT > 0;
        int validT1 = int(validTBool1);
        int inValidT1 = int(!validTBool1);

        currentT = (validT1 * min(currentT, bodyT)) + (inValidT1 * bodyT);
    }

    // top cap
    vec3 topNorm = vec3(0.f, 1.f, 0.f);
    vec3 topPt = vec3(0.f, .5, 0.f);
    float topT = cap(objPeye, objD, topNorm, topPt);

    if (topT >= 0) {
        if (currentT > 0) {
            currentT = min(currentT, topT);
        } else {
            currentT = topT;
        }
    }

    // bottom cap
    vec3 bottomNorm = vec3(0.f, -1.f, 0.f);
    vec3 bottomPt = vec3(0.f, -.5, 0.f);
    float bottomT = cap(objPeye, objD, bottomNorm, bottomPt);

    if (bottomT >= 0) {

        if (currentT > 0) {
            currentT = min(currentT, bottomT);
        } else {
            currentT = bottomT;
        }
    }
    return currentT;
}

// helper method for cone
float coneBody(vec4 objPeye, vec4 objD)
{
    float a = pow(objD.x, 2.0) + pow(objD.z, 2.0) - (.25 * pow(objD.y, 2.0));

    float b = (2.0 * objPeye.x * objD.x) + (2.0 * objPeye.z * objD.z) -
            (.5 * objPeye.y * objD.y) + (.25 * objD.y);

    float c = pow(objPeye.x, 2.0) + pow(objPeye.z, 2.0) -
            (.25 * pow(objPeye.y, 2.0)) + (.25 * objPeye.y) - (1.0/16.0);

    float discriminant = pow(b, 2.0) - (4.0 * a * c);
    if (discriminant >= 0) {
        float t1 = ((-1.0 * b) + sqrt(discriminant))/(2.0 * a);
        float t2 = ((-1.0 * b) - sqrt(discriminant))/(2.0 * a);

        float y1 = objPeye.y + (t1 * objD.y);
        float y2 = objPeye.y + (t2 * objD.y);
        bool y1Valid = y1 >= -.5 && y1 <= .5;
        bool y2Valid = y2 >= -.5 && y2 <= .5;

        if (y1Valid && y2Valid) {
            if (t1 > 0 && t2 > 0) {
                return min(t1, t2);
            } else if (t1 <= 0 && t2 <= 0) {
                return -1.0;
            } else {
                return max(t1, t2);
            }
        } else if (y1Valid) {
            if (t1 > 0) {
                return t1;
            } else {
                return -1.0;
            }
        } else if (y2Valid){
            if (t2 > 0) {
                return t2;
            } else {
                return -1.0;
            }
        } else {
            return -1.0;
        }
    } else {
        return -1.0;
    }
}

// finds the intersection 't' of the input ray with a cone centered at (0,0,0)
// with r=.5 and h=1
// objPeye and objD must already be in object space.
float cone(vec4 objPeye, vec4 objD)
{
    float currentT = -1.f;

    float bodyT = coneBody(objPeye, objD);
    if (bodyT >= 0) {
        if (currentT > 0) {
            currentT = min(currentT, bodyT);
        } else {
            currentT = bodyT;
        }
    }

    vec3 capNorm = vec3(0.f, -1.f, 0.f);
    vec3 capPt = vec3(0.f, -.5, 0.f);
    float capT = cap(objPeye, objD, capNorm, capPt);

    if (capT >= 0) {
        if (currentT > 0) {
            currentT = min(currentT, capT);
        } else {
            currentT = capT;
        }
    }
    return currentT;
}

float plane(vec4 objPeye, vec4 objD, vec3 normal, vec3 pt, int planeSignal)
{
    float tDenom = (normal.x * objD.x) + (normal.y * objD.y) + (normal.z * objD.z);
    float tNum1 = (normal.x * pt.x) + (normal.y * pt.y) + (normal.z * pt.z);
    float tNum2 = (normal.x * objPeye.x) + (normal.y * objPeye.y) + (normal.z * objPeye.z);
    float t = (tNum1 - tNum2)/tDenom;
    vec4 intersectionPt = objPeye + (t * objD);


    int isXplane = int(planeSignal == 0);
    int isYplane = int(planeSignal == 1);
    int isZplane = int(planeSignal == 2);

    bool xPlaneIntersection = (intersectionPt.y >= -.5 &&
                              intersectionPt.y <= .5 &&
                              intersectionPt.z >= -.5 &&
                              intersectionPt.z <= .5);
    int intersectsXPlane = int(xPlaneIntersection);
    int noIntersectsXplane = int(!xPlaneIntersection);

    bool yPlaneIntersection = (intersectionPt.x >= -.5 &&
                               intersectionPt.x <= .5 &&
                               intersectionPt.z >= -.5 &&
                               intersectionPt.z <= .5);

    int yPlaneIntersects = int(yPlaneIntersection);
    int yPlaneNoIntersects = int(!yPlaneIntersection);

    bool zPlaneIntersection = (intersectionPt.x >= -.5 &&
                               intersectionPt.x <= .5 &&
                               intersectionPt.y >= -.5 &&
                               intersectionPt.y <= .5);
    int zPlaneIntersects = int(zPlaneIntersection);
    int zPlaneNoIntersects = int(!zPlaneIntersection);


    return (isXplane * ((intersectsXPlane * t) + (noIntersectsXplane * -1.0))) +
            (isYplane * ((yPlaneIntersects * t) + (yPlaneNoIntersects * -1.0))) +
            (isZplane * ((zPlaneIntersects * t) + (zPlaneNoIntersects * -1.0)));
}

float cube(vec4 objPeye, vec4 objD)
{

    float currentT = -1.f;

    // --------- X+ ----------------
    // positive x plane
    vec3 xPosNorm = vec3(1.f, 0.f, 0.f);
    vec3 xPosPt = vec3(.5, 0.f, 0.f);
    float xPosT = plane(objPeye, objD, xPosNorm, xPosPt, 0);

    // check for intersection with x-Positive plane
    bool xPosIntersection = xPosT >= 0;
    int xPosIntersect = int(xPosIntersection);
    int xPosNoIntersect = int(!xPosIntersection);

    bool prevIntersectBool1 = currentT > 0;

    int prevIntersect1 = int(prevIntersectBool1);
    int noPrevIntersect1 = int(!prevIntersectBool1);

    currentT = (xPosIntersect * (prevIntersect1 * min(xPosT, currentT)) + (noPrevIntersect1 * xPosT)) +
                (xPosNoIntersect * currentT);

    // --------- X- ----------------

    // negative x plane
    vec3 xNegNorm = vec3(-1.f, 0.f, 0.f);
    vec3 xNegPt = vec3(-.5, 0.f, 0.f);
    float xNegT = plane(objPeye, objD, xNegNorm, xNegPt, 0);

    // check for intersection with x-Negative plane
    bool xNegIntersection = xNegT >= 0;
    int xNegIntersect = int(xNegIntersection);
    int xNegNoIntersect = int(!xNegIntersection);

    bool prevIntersectBool2 = currentT > 0;
    int prevIntersect2 = int(prevIntersectBool2);
    int noPrevIntersect2 = int(!prevIntersectBool2);

    currentT = (xNegIntersect * (prevIntersect2 * min(xNegT, currentT)) + (noPrevIntersect2 * xNegT)) +
                (xNegNoIntersect * currentT);

    // --------- Y+ ----------------

    // positive y plane
    vec3 yPosNorm = vec3(0.f, 1.f, 0.f);
    vec3 yPosPt = vec3(0.f, .5, 0.f);
    float yPosT = plane(objPeye, objD, yPosNorm, yPosPt, 1);

    // check for intersection with y-Pos plane
    bool yPosIntersection = yPosT >= 0;
    int yPosIntersect = int(yPosIntersection);
    int yPosNoIntersect = int(!yPosIntersection);

    bool prevIntersectBool3 = currentT > 0;
    int prevIntersect3 = int(prevIntersectBool3);
    int noPrevIntersect3 = int(!prevIntersectBool3);

    currentT = (yPosIntersect * (prevIntersect3 * min(yPosT, currentT)) + (noPrevIntersect3 * yPosT)) +
                (yPosNoIntersect * currentT);


    // --------- Y- ----------------

    // negative y plane
    vec3 yNegNorm = vec3(0.f, -1.f, 0.f);
    vec3 yNegPt = vec3(0.f, -.5, 0.f);
    float yNegT = plane(objPeye, objD, yNegNorm, yNegPt, 1);

    // check for intersection with y-Neg plane
    bool yNegIntersection = yNegT >= 0;
    int yNegIntersect = int(yNegIntersection);
    int yNegNoIntersect = int(!yNegIntersection);

    bool prevIntersectBool4 = currentT > 0;
    int prevIntersect4 = int(prevIntersectBool4);
    int noPrevIntersect4 = int(!prevIntersectBool4);

    currentT = (yNegIntersect * (prevIntersect4 * min(yNegT, currentT)) + (noPrevIntersect4 * yNegT)) +
                (yNegNoIntersect * currentT);

    // --------- Z+ ----------------

    //positive z plane
    vec3 zPosNorm = vec3(0.f, 0.f, 1.f);
    vec3 zPosPt = vec3(0.f, 0.f, .5);
    float zPosT = plane(objPeye, objD, zPosNorm, zPosPt, 2);

    // check for intersection with z-Pos plane
    bool zPosIntersection = zPosT >= 0;
    int zPosIntersect = int(zPosIntersection);
    int zPosNoIntersect = int(!zPosIntersection);

    bool prevIntersectBool5 = currentT > 0;
    int prevIntersect5 = int(prevIntersectBool5);
    int noPrevIntersect5 = int(!prevIntersectBool5);

    currentT = (zPosIntersect * (prevIntersect5 * min(zPosT, currentT)) + (noPrevIntersect5 * zPosT)) +
                (zPosNoIntersect * currentT);


    // --------- Z- ----------------

    // negative z plane
    vec3 zNegNorm = vec3(0.f, 0.f, -1.f);
    vec3 zNegPt = vec3(0.f, 0.f, -.5);
    float zNegT = plane(objPeye, objD, zNegNorm, zNegPt, 2);

    // check for intersection with z-Neg plane
    bool zNegIntersection = zNegT >= 0;
    int zNegIntersect = int(zNegIntersection);
    int zNegNoIntersect = int(!zNegIntersection);

    bool prevIntersectBool6 = currentT > 0;
    int prevIntersect6 = int(prevIntersectBool6);
    int noPrevIntersect6 = int(!prevIntersectBool6);

    currentT = (zNegIntersect * (prevIntersect6 * min(zNegT, currentT)) + (noPrevIntersect6 * zNegT)) +
                (zNegNoIntersect * currentT);

    return currentT;
}

// Based on object space intersection point, get normal
vec3 sphereNormal(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 start = vec3(0.0);
    return normalize(vec3(point) - start);
}

vec3 cylinderNormal(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 normal = vec3(0.0);

    if (abs(point.y - 0.5) < SHAPE_EPSILON){ // top cap
        normal = vec3(0.f, 1.f, 0.f);
    }else if (abs(point.y - (-0.5)) < SHAPE_EPSILON){ // bottom cap
        normal = vec3(0.f, -1.f, 0.f);
    // on the drum
    }else{
        // normal is just from the center axis (at same y value), to the point of intersection, normalized
        vec4 unNormalized = point - vec4(0.0, point.y, 0.0, 1.0);
        vec4 normal4 = normalize(unNormalized);
        normal = vec3(normal4.x, normal4.y, normal4.z);
    }
    return normal;
}

vec3 coneNormal(vec4 p, vec4 d, float t){

    vec4 point = p + t*d;
    vec3 normal = vec3(0.f);
    // if intersection is on cap, handle differently
    // at this point, we know t is intersecting the object
    if (abs(point.y - (-0.5)) < SHAPE_EPSILON){ // bottom cap
        normal = vec3(0.f, -1.f, 0.f);
    // on the cone body
    }else{
        // calculate x and z as if the normal is for a cylinder first
        // and then use the length of that as the "tempradius" to calculate the y component
        vec3 cylStart = vec3(0.f, point.y, 0.f);
        vec3 cylNormal = vec3(point) - cylStart; // don't normalize yet, b/c we need the object space distance from axis to surface
        float a = length(cylNormal); // a is the tempRadius
        // the ratio of rise to run is the SLOPE
        float yComponent = a/CONE_SLOPE;
        normal = normalize(vec3(cylNormal.x, cylNormal.y + yComponent, cylNormal.z));
    }
    return normal;
}

// returns normal of cube in object space
vec3 cubeNormal(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 normal = vec3(0.f);
    // handle differently depending on what face it's on
    // at this point, we know t is intersecting the object
    if (point.x >= (.5 - SHAPE_EPSILON)) {
        // x+ plane
        normal = vec3(1.0, 0.0, 0.0);
    } else if (point.x <= (-.5 + SHAPE_EPSILON)) {
        // x- plane
        normal = vec3(-1.0, 0.0, 0.0);
    } else if (point.y >= (.5 - SHAPE_EPSILON)) {
        // y+ plance
        normal = vec3(0.0, 1.0, 0.0);
    } else if (point.y <= (-.5 + SHAPE_EPSILON)) {
        // y- plane
        normal = vec3(0.0, -1.0, 0.0);
    } else if (point.z >= (.5 - SHAPE_EPSILON)) {
        // z+ plane
        normal = vec3(0.0, 0.0, 1.0);
    } else {
        // z- plane
        normal = vec3(0.0, 0.0, -1.0);
    }
    return normal;
}

// Based on object space intersection point, get bitangent
vec3 sphereBitangent(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 bitangent = vec3(0.f);

    if (abs(point.y - 0.5) < SHAPE_EPSILON){ // top pole
        bitangent = vec3(0.f, 0.f, -1.f);
    }else if (abs(point.y - (-0.5)) < SHAPE_EPSILON){ // bottom pole
        bitangent = vec3(0.f, 0.f, 1.f);
    // on the surface
    }else{
        // bitangent is just the y axis
        bitangent = vec3(0.f, 1.f, 0.f);
    }
    return bitangent;
}

// Based on object space intersection point, get bitangent
vec3 cubeBitangent(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 bitangent = vec3(0.f);
    // handle differently depending on what face it's on
    // at this point, we know t is intersecting the object

    if (point.x >= (.5 - SHAPE_EPSILON)) {
        // x+ plane
        bitangent = vec3(0.0, 1.0, 0.0);
    } else if (point.x <= (-.5 + SHAPE_EPSILON)) {
        // x- plane
        bitangent = vec3(0.0, 1.0, 0.0);
    } else if (point.y >= (.5 - SHAPE_EPSILON)) {
        // y+ plane
        bitangent = vec3(0.0, 0.0, -1.0);
    } else if (point.y <= (-.5 + SHAPE_EPSILON)) {
        // y- plane
        bitangent = vec3(0.0, 0.0, 1.0);
    } else if (point.z >= (.5 - SHAPE_EPSILON)) {
        // z+ plane
        bitangent = vec3(0.0, 1.0, 0.0);
    } else {
        // z- plane
        bitangent = vec3(0.0, 1.0, 0.0);
    }
    return bitangent;
}

// Based on object space intersection point, get bitangent
vec3 coneBitangent(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 bitangent = vec3(0.0);

    if (abs(point.y - (-0.5)) < SHAPE_EPSILON){ // bottom cap
        bitangent = vec3(0.f, 0.f, 1.f);
    // on the drum
    }else{
        // bitangent is just the y axis
        bitangent = vec3(0.f, 1.f, 0.f);
    }
    return bitangent;
}

// Based on object space intersection point, get bitangent
vec3 cylinderBitangent(vec4 p, vec4 d, float t){
    vec4 point = p + t*d;
    vec3 bitangent = vec3(0.0);

    if (abs(point.y - 0.5) < SHAPE_EPSILON){ // top cap
        bitangent = vec3(0.f, 0.f, -1.f);
    }else if (abs(point.y - (-0.5)) < SHAPE_EPSILON){ // bottom cap
        bitangent = vec3(0.f, 0.f, 1.f);
    // on the drum
    }else{
        // bitangent is just the y axis
        bitangent = vec3(0.f, 1.f, 0.f);
    }
    return bitangent;
}

// Get Cube UV coordinates, based on an object space intersection point
vec2 cubeUV(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t){
    vec4 objSpaceIntersectionPt = objectSpacePoint + (t * objectSpaceDirection);

    vec2 toReturn;
    float uVal;
    float vVal;

    if (objSpaceIntersectionPt.x >= (.5 - SHAPE_EPSILON)) {
        //x+ plane
        uVal = 1.f - (objSpaceIntersectionPt.z + .5);
        vVal = 1.f - (objSpaceIntersectionPt.y + .5);
    } else if (objSpaceIntersectionPt.x <= (-.5 + SHAPE_EPSILON)) {
        // x- plane
        uVal = objSpaceIntersectionPt.z + .5;
        vVal = 1.f - (objSpaceIntersectionPt.y + .5);
    } else if (objSpaceIntersectionPt.y >= (.5 - SHAPE_EPSILON)) {
        // y+ plane
        uVal = objSpaceIntersectionPt.x + .5;
        vVal = objSpaceIntersectionPt.z + .5;
    } else  if (objSpaceIntersectionPt.y <= (-.5 + SHAPE_EPSILON)){
        // y- plane
        uVal = objSpaceIntersectionPt.x + .5;
        vVal = 1.f - (objSpaceIntersectionPt.z + .5);
    } else if (objSpaceIntersectionPt.z >= (.5 - SHAPE_EPSILON)) {
        // z+ plane
        uVal = objSpaceIntersectionPt.x + .5;
        vVal = 1.f - (objSpaceIntersectionPt.y + .5);

    } else {
        // z- plane
        uVal = 1.f - (objSpaceIntersectionPt.x + .5);
        vVal = 1.f - (objSpaceIntersectionPt.y + .5);
    }

    uVal = clamp(uVal, 0.0, 1.0);
    vVal = clamp(vVal, 0.0, 1.0);

    return vec2(uVal, vVal);
}

// Get Cone UV coordinates, based on an object space intersection point
vec2 coneUV(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t){
    vec4 intersection = objectSpacePoint + t * objectSpaceDirection;
    vec2 uv = vec2(0.0);
    // If intersection on cap, handle UV's like a plane
    if (abs(intersection.y - (-0.5)) < SHAPE_EPSILON){ // bottom cap
        uv = vec2(intersection.x + 0.5, 1.f - (intersection.z + 0.5));
    }else{ // else on the drum
        float theta = atan(intersection.z, intersection.x);
        float arcLength;
        if (theta < 0.0){
            arcLength = -theta/(2 * PI);
        }else{
            arcLength = 1.f - theta/(2 * PI);
        }
        uv = vec2(arcLength, 1.0 - (intersection.y + 0.5));
    }
    // Clamp UVs between 0 and 1
    uv = vec2(max(0.0, min(1.0, uv[0])), max(0.0, min(1.0, uv[1])));
    return uv;
}

// Get sphere UV coordinates, based on an object space intersection point
vec2 sphereUV(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t){
    vec4 point = objectSpacePoint + t*objectSpaceDirection;
    vec2 uv = vec2(0.f);

    // At poles (v = 0 or v = 1), no u value. let u = arbitrary 0.5
    if (abs(point.y - 0.5) < SHAPE_EPSILON){ // top pole
        uv = vec2(0.5, 1.f);
    }else if (abs(point.y - (-0.5)) < SHAPE_EPSILON){ // bottom pole
        uv = vec2(0.5, 0.f);
    // surface
    }else{
        // u is calculated the same way as Cylinder/Cone
        float theta = atan(point.z, point.x);
        // Need to check if this is above or below x axis
        float u;
        if (theta < 0.f){
            u = -theta/(2 * PI);
        }else{
            u = 1.f - theta/(2*PI);
        }
        // v is calculated as a function phi of the point
        float phi = asin(point.y/0.5); // Radius is 0.5;
        float v = (phi / PI) + 0.5;
        // Considering (0, 0) to be top left corner of the texture, UV map, so reverse this v
        v = 1.f - v;

        uv = vec2(u, v);
    }
    // Clamp UVs between 0.f and 1.f
    uv = vec2(max(0.f, min(1.f, uv[0])), max(0.f, min(1.f, uv[1])));

    return uv;
}

// Get Cylinder UV coordinates, based on an object space intersection point
vec2 cylinderUV(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t){
    vec4 intersection = objectSpacePoint + t * objectSpaceDirection;
    vec2 uv = vec2(0.0);
    // If intersection on cap, handle UV's like a plane
    if (abs(intersection.y - 0.5) < SHAPE_EPSILON){ // top cap
        uv = vec2(intersection.x + 0.5, intersection.z + 0.5);
    }else if (abs(intersection.y - (-0.5)) < SHAPE_EPSILON){ // bottom cap
        uv = vec2(intersection.x + 0.5, 1.f - (intersection.z + 0.5));
    }else{ // else on the drum
        float theta = atan(intersection.z, intersection.x);
        float arcLength;
        if (theta < 0.0){
            arcLength = -theta/(2 * PI);
        }else{
            arcLength = 1.f - theta/(2 * PI);
        }
        uv = vec2(arcLength, 1.0 - (intersection.y + 0.5));
    }
    // Clamp UVs between 0 and 1
    uv = vec2(max(0.0, min(1.0, uv[0])), max(0.0, min(1.0, uv[1])));
    return uv;
}

// [RAY TRACING]
/////////////////////////////////////////////////////////////////////////

// Random number for noise
// Returns a pseudo-random value between 0.0 and 1.0
// Using single seed
float randValue1(float seed) {
    return -1.0 + 2.0 * fract(sin(seed * 127.1f + -seed * 311.7f) * 43758.5453123f);
}
// Returns a pseudo-random value between 0.0 and 1.0
// Using 2 seeds
float randValue2(float seed1, float seed2) {
    return fract(sin(dot(vec2(seed1, seed2), vec2(12.9898, 78.233))) * 43758.5453);
}

// Get light vector, based on lightObject struct point and a world space point of intersection
vec4 getLightVector(LightObject lightObject, vec4 worldSpaceIntersection){
    vec4 lightVector = vec4(0.0);
    if (lightObject.type == LIGHT_POINT){
        lightVector = (lightObject.pos - worldSpaceIntersection);
    }else if (lightObject.type == LIGHT_DIRECTIONAL){
        lightVector = -normalize(lightObject.dir);
    }
    return lightVector;
}

// Given a light in the scene and the offsetIntersectionPoint, calculate distance from the object to the light source
float getLightDistance(LightObject light, vec4 offsetIntersection){
    float infinity = 1.0/0.0; // Infinity - in GLSL, IEEE 754 infinity generated by dividing by zero
    float distance = infinity;
    if (light.type == LIGHT_POINT){
        distance = length(offsetIntersection - light.pos);
    }
    // If directional light, distance infinity
    return distance;
}

// Given an object space point/direction and primitive type, test if there is an intersection
// Returns a PrimitiveType struct, with the t value, and object information.
// intersectionType and t will tell if there has been an intersection or not
PrimitiveType checkObjectIntersection(vec4 objectSpacePoint, vec4 objectSpaceDirection, SceneObject obj){
    float closestT = -1.0;
    int intersectionType = NO_INTERSECT;
    float objectT = -1.0;
    if (obj.primitive == SPHERE){
        objectT = sphere(objectSpacePoint, objectSpaceDirection);
    }else if (obj.primitive == CONE){
        objectT = cone(objectSpacePoint, objectSpaceDirection);
    }else if (obj.primitive == CYLINDER){
        objectT = cylinder(objectSpacePoint, objectSpaceDirection);
    }else if (obj.primitive == CUBE){
        objectT = cube(objectSpacePoint, objectSpaceDirection);
    }
    if (objectT > 0.0){
        closestT = objectT;
        intersectionType = obj.primitive;
    }
    return PrimitiveType(closestT, intersectionType,
                         obj.cDiffuse,
                         obj.cAmbient,
                         obj.cSpecular,
                         obj.cReflective,
                         obj.shininess,
                         obj.objectToWorld,
                         obj.blend,
                         obj.texID,
                         obj.repeatU,
                         obj.repeatV);
}

// returns a primitiveType of the intersected object in the scene.
PrimitiveType getIntersection(vec4 worldSpacePoint, vec4 worldSpaceDir)
{
    vec4 objectSpacePoint = vec4(0.0);
    vec4 objectSpaceDirection = vec4(0.0);

    float bestT = -1.0;
    PrimitiveType intersectObject = PrimitiveType(bestT, NO_INTERSECT,
                                                  vec4(0.0), vec4(0.0),
                                                  vec4(0.0), vec4(0.0), 0.0,
                                                  mat4x4(1.0), 0.0, 0, 0.0, 0.0);


    for (int i = 0; i < sceneObjects.length(); i++) {

        SceneObject curObj = sceneObjects[i];
        objectSpacePoint = inverse(curObj.objectToWorld) * worldSpacePoint;
        objectSpaceDirection = inverse(curObj.objectToWorld) * worldSpaceDir;

        PrimitiveType currentIntersection = checkObjectIntersection(objectSpacePoint,
                                                                    objectSpaceDirection, curObj);

        // compare current intersection object with best intersection object
        if (currentIntersection.t >= 0.0) {
            if (bestT >= 0) {
                // compare to current best intersection. both bestT and currentT are valid

                if (bestT > currentIntersection.t) {
                    // currentIntersection is closer;
                    bestT = currentIntersection.t;
                    intersectObject = currentIntersection;
                }
            } else {
                // currentIntersection is best option
                bestT = currentIntersection.t;
                intersectObject = currentIntersection;
            }
        }
    }
    return intersectObject;
}

// Get object bitangent, based on object space intersection point
// y facing up
vec3 getObjectBitangent(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t, int primitive)
{
    int isSphere = int(primitive == SPHERE);
    int isCone = int(primitive == CONE);
    int isCylinder = int(primitive == CYLINDER);
    int isCube = int(primitive == CUBE);

    // condensed if/else statement
    return (isSphere * sphereBitangent(objectSpacePoint, objectSpaceDirection, t)) +
            (isCone * coneBitangent(objectSpacePoint, objectSpaceDirection, t)) +
            (isCylinder * cylinderBitangent(objectSpacePoint, objectSpaceDirection, t)) +
            (isCube * cubeBitangent(objectSpacePoint, objectSpaceDirection, t));
}

// Get object tangent, based on object space intersection point
// x facing horizontally
vec3 getObjectTangent(vec3 normal, vec3 bitangent)//vec4 objectSpacePoint, vec4 objectSpaceDirection, float t, int primitive)
{
    return cross(bitangent, normal);
}

// Get object normal, based on object space intersection point
// 'z' facing towards us
vec3 getObjectNormal(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t, int primitive)
{
    int isSphere = int(primitive == SPHERE);
    int isCone = int(primitive == CONE);
    int isCylinder = int(primitive == CYLINDER);
    int isCube = int(primitive == CUBE);

    // condensed if/else statement
    return (isSphere * sphereNormal(objectSpacePoint, objectSpaceDirection, t)) +
            (isCone * coneNormal(objectSpacePoint, objectSpaceDirection, t)) +
            (isCylinder * cylinderNormal(objectSpacePoint, objectSpaceDirection, t)) +
            (isCube * cubeNormal(objectSpacePoint, objectSpaceDirection, t));

}

// Get object UV. based on object space intersection point
vec2 getObjectUV(vec4 objectSpacePoint, vec4 objectSpaceDirection, float t, int primitive)
{
    int isSphere = int(primitive == SPHERE);
    int isCone = int(primitive == CONE);
    int isCylinder = int(primitive == CYLINDER);
    int isCube = int(primitive == CUBE);

    // condensed if/else statement
    return (isSphere * sphereUV(objectSpacePoint, objectSpaceDirection, t)) +
            (isCone * coneUV(objectSpacePoint, objectSpaceDirection, t)) +
            (isCylinder * cylinderUV(objectSpacePoint, objectSpaceDirection, t)) +
            (isCube * cubeUV(objectSpacePoint, objectSpaceDirection, t));

}

// Tangent to object space transformation matrix
// TBN (Tangent, BiTangent, Normal)
mat3x3 tangentToObject(vec3 objectSpaceTangent, vec3 objectSpaceBitangent, vec3 objectSpaceNormal){
    return mat3(objectSpaceTangent, objectSpaceBitangent, objectSpaceNormal);
}

// returns the world space intersection point of an object and the ray that hit it
vec4 getWorldSpaceIntersectionPt(PrimitiveType obj, vec4 worldSpacePoint, vec4 worldSpaceDir)
{
    vec4 objSpaceIntersectionPt = (inverse(obj.objectToWorld) * worldSpacePoint) +
                                    (obj.t * (inverse(obj.objectToWorld) * worldSpaceDir));

    return obj.objectToWorld * objSpaceIntersectionPt;
}

// checks for shadow intersection and returns appropriate light color.
// if an object obstructs the given object and light, returns black.
// otherwise returns light color.
vec4 getLightContribution(PrimitiveType obj, vec4 worldPoint, vec4 worldDirection, vec4 worldSpaceNormal, LightObject light)
{
    vec4 worldSpaceIntersectionPt = worldPoint + obj.t * worldDirection;

    // raise world space intersection point by epsilon along normal
    vec4 raisedStartPt = worldSpaceIntersectionPt + (SHAPE_EPSILON * worldSpaceNormal);

    // get ray going from world space intersection pt to light position
    vec4 rayToLight = getLightVector(light, worldSpaceIntersectionPt); //normalize(light.pos - worldSpaceIntersectionPt);

    // distance to light from object
    float maxDistance = getLightDistance(light, raisedStartPt);

    // check for intersection with objects
    PrimitiveType obstructingObj = getIntersection(raisedStartPt, rayToLight);

    // find distance between object and obstructing object
    vec4 currIntersection = getWorldSpaceIntersectionPt(obstructingObj, raisedStartPt, rayToLight);
    float currDistance = length(raisedStartPt - currIntersection);

    // Check if closest intersected object is in front of or behind the light source
    int shadowCastingTrue = int(obstructingObj.t > 0 && currDistance < maxDistance);
    int shadowCastingFalse = abs(shadowCastingTrue-1);

    vec4 shadowColor = vec4(0.0, 0.0, 0.0, 1.0);
    return (shadowCastingTrue * shadowColor) + (shadowCastingFalse * light.color);
}

// Texture Mapping
// Sample a texture, given a material's textureMap and a object space intersection point
vec4 sampleTexture(vec4 objectSpacePoint, vec4 objectSpaceDirection, PrimitiveType obj, int textureType)
{
    int type = obj.primitive;
    int texID = obj.texID;
    float t = obj.t;
    vec4 textureColor = vec4(0.f);

    float j = obj.repeatU;
    float k = obj.repeatV;

    float w = 1.0;
    float h = 1.0;
    vec2 uv = getObjectUV(objectSpacePoint, objectSpaceDirection, t, type);
    float uIndex = uv[0];
    float vIndex = uv[1];

    float sIndex = mod((uIndex * j * w), w);
    float tIndex = mod((vIndex * k * h), h);

    if (texID == 0) {
        // metal diffuse texture
        if (textureType == NORMAL) {
            textureColor = vec4(texture(metalNormalTex, vec2(sIndex, tIndex)).bgr, 1.0);
        } else {
            textureColor = vec4(texture(metalDiffuseTex, vec2(sIndex, tIndex)).bgr, 1.0);
        }
    } else if (texID == 1) {
        // wood diffuse texture
        if (textureType == NORMAL) {
            textureColor = vec4(texture(woodNormalTex, vec2(sIndex, tIndex)).bgr, 1.0);
        } else {
            textureColor = vec4(texture(woodDiffuseTex, vec2(sIndex, tIndex)).bgr, 1.0);
        }
    }else if (texID == 2) {
        // wood diffuse texture
        if (textureType == NORMAL) {
            textureColor = vec4(texture(plasterNormalTex, vec2(sIndex, tIndex)).bgr, 1.0);
        } else {
            textureColor = vec4(texture(plasterDiffuseTex, vec2(sIndex, tIndex)).bgr, 1.0);
        }
    }

    return textureColor;
}

vec4 getNormalMappedNormal(PrimitiveType obj, vec4 worldNormal, vec4 worldPoint, vec4 worldDirection)
{
    vec4 objectSpacePoint = inverse(obj.objectToWorld) * worldPoint;
    vec4 objectSpaceDirection = inverse(obj.objectToWorld) * worldDirection;

    mat3x3 worldToObject = inverse(transpose(mat3x3(inverse(obj.objectToWorld))));
    vec3 objectSpaceNormal = worldToObject * vec3(worldNormal);
    vec3 objectSpaceBitangent = getObjectBitangent(objectSpacePoint, objectSpaceDirection, obj.t, obj.primitive);
    vec3 objectSpaceTangent = getObjectTangent(objectSpaceNormal, objectSpaceBitangent);
    mat3x3 tangentToObject = tangentToObject(objectSpaceTangent, objectSpaceBitangent, objectSpaceNormal);

    // Sample normal map
    vec4 textureColor = sampleTexture(objectSpacePoint, objectSpaceDirection, obj, NORMAL);//obj.t, obj.primitive, NORMAL, obj.texID);
    // Remap tangent space texture data into normal domain [-1, 1]
    float x = (textureColor.r * 2.0) - 1.0;
    float y = (textureColor.g * 2.0) - 1.0;
    float z = (textureColor.b * 2.0) - 1.0;
    textureColor = vec4(x, y, z, 0.0);
    vec3 tangentNormal = vec3(textureColor);

    // Convert tangent space to object space to normal space
    objectSpaceNormal = tangentToObject * tangentNormal;
    mat3x3 objectToWorld = transpose(mat3x3(inverse(obj.objectToWorld)));
    worldNormal = vec4(objectToWorld * objectSpaceNormal, 0.0);

    return worldNormal;
}

// The primary lighting equation
// Calculate lighting for this material at this intersection point
vec4 calculateLighting(vec4 worldNormal, vec4 worldPoint, vec4 worldDirection, PrimitiveType obj){

    vec4 worldIntersection = worldPoint + (obj.t * worldDirection);
    vec4 objectSpacePoint = inverse(obj.objectToWorld) * worldPoint;
    vec4 objectSpaceDirection = inverse(obj.objectToWorld) * worldDirection;

    // Object material constants
    vec4 objAmb = obj.cAmbient;
    vec4 objSpec = obj.cSpecular;
    vec4 objDiffuse = globalData.kd * obj.cDiffuse; // apply global diffuse before we texture map

    // --------- AMBIENT ---------
    int usingAmbient = int(settings.useAmbient == 1);
    int notUsingAmbient = int(settings.useAmbient != 1);
    // Scale ambient by the average of the three light intensities
    vec4 ambient = (usingAmbient * (globalData.ka * objAmb)) + (notUsingAmbient * vec4(0.0));

    // --------- TEXTURE MAPPING ---------
    // If using texture mapping and material has a texture map
    if (settings.useTextures == 1){
        vec4 objectSpacePoint = inverse(obj.objectToWorld) * worldPoint;
        vec4 objectSpaceDirection = inverse(obj.objectToWorld) * worldDirection;

        vec4 textureColor = sampleTexture(objectSpacePoint, objectSpaceDirection, obj, DIFFUSE);
        float blend = obj.blend;
        objDiffuse = blend * textureColor + (1.f - blend) * objDiffuse;
    }

    // --------- NORMAL MAPPING ---------
    // If normal mapping, use Nu Nv Nw (r, g, b) in tangent space instead of the worldNormal argument
    if (settings.useNM == 1 && obj.blend > 0.0){
        worldNormal = getNormalMappedNormal(obj, worldNormal, worldPoint, worldDirection);
    }

    vec4 sum = vec4(0.0);
    float lightIntensitySum = 0.0;

    // For each light in the scene, calculate lighting contribution
    for (int i = 0; i < sceneLights.length(); i++) {

        // from intersection point to light. should NOT be normalized.
        vec4 lightVec = getLightVector(sceneLights[i], worldIntersection);
        vec3 lightFunction = sceneLights[i].function;

        // --------- SHADOWS ---------
        // aka: if using shadows, get light intenisty from 'getLightContribution', if not, use sceneLights[i].color
        int usingShadows = int(settings.useShadows == 1);
        int notUsingShadows = int(settings.useShadows != 1);
        vec4 lightIntensity = (usingShadows * getLightContribution(obj, worldPoint, worldDirection, worldNormal, sceneLights[i])) +
                (notUsingShadows * sceneLights[i].color);

        // Scale lightIntensity by UI setting
        lightIntensity *= sceneLights[i].lightIntensitySetting;
        lightIntensitySum += sceneLights[i].lightIntensitySetting;

        // --------- DIFFUSE ---------
        vec4 noDiffuse = vec4(0.0);
        vec4 withDiffuse = objDiffuse * vec4(clamp(dot(vec3(worldNormal), normalize(vec3(lightVec))), 0.0, 1.0));

        int usingDiffuse = int(settings.useDiffuse == 1);
        int notUsingDiffuse = int(settings.useDiffuse != 1);

        vec4 diffuse = (withDiffuse * usingDiffuse) + (notUsingDiffuse * noDiffuse);

        // --------- SPECULAR ---------

        vec4 reflectedLightRay = reflect(-normalize(lightVec), worldNormal);       // Pointing away from object
        vec4 lineOfSight = worldPoint - worldIntersection;
        float specularDot = clamp(dot(normalize(reflectedLightRay), normalize(lineOfSight)), 0.0, 1.0);

        vec4 withSpec = clamp(objSpec * globalData.ks * pow(specularDot, obj.shininess), 0.0, 1.0);
        vec4 noSpec = vec4(0.0);

        int usingSpec = int(settings.useSpecular == 1);
        int notUsingSpec = int(settings.useSpecular != 1);

        vec4 specular = (withSpec * usingSpec) + (noSpec * notUsingSpec);

        // --------- LIGHT ATTENUATION ---------
        // only applicable if light is NOT a directional light
        float lightDistance = length(lightVec);
        float lightAttenuation = 1.f;
        if (sceneLights[i].type != LIGHT_DIRECTIONAL){
            lightAttenuation = 1.0 /
                    (lightFunction[0]
                    + lightFunction[1] * lightDistance +
                    + lightFunction[2] * pow(lightDistance, 2.0f));
        }

        vec4 currIntensity = lightAttenuation * lightIntensity * (diffuse + specular);
        sum += currIntensity;
    }
    // End for loop

    // Scale ambient by average of lightIntensities from UI
    float avgLightIntensity = lightIntensitySum / sceneLights.length();

    return (avgLightIntensity * ambient) + sum;
}

// returns the world space normal from an intersection point of the given world space ray and obj.
vec4 getWorldSpaceNormal(PrimitiveType obj, vec4 worldSpacePoint, vec4 worldSpaceDir)
{
    vec4 objSpacePoint = inverse(obj.objectToWorld) * worldSpacePoint;
    vec4 objSpaceDir = inverse(obj.objectToWorld) * worldSpaceDir;

    vec4 objNormal = vec4(getObjectNormal(objSpacePoint,
                                          objSpaceDir,
                                          obj.t,
                                          obj.primitive), 0.0);

    mat3x3 objectToWorld = transpose(mat3x3(inverse(obj.objectToWorld)));

    vec4 worldNormal = vec4(objectToWorld * vec3(objNormal), 0.0);
    return worldNormal;
}


// returns a 4x4 matrix that will rotate the input vector
// to the z axis.
mat4x4 getZaxisAlignmentRotation(vec4 worldNormal)
{
    if (worldNormal.z == 1.f) {
        // normal is already z+, no transformation necessary
        return mat4x4(1.f);
    } else if (worldNormal.z == -1.f) {

        // normal is in z- direction, rotate by 180 about x axis
        float phi = PI;
        mat4x4 zNegMat = mat4x4(1.0, 0.0, 0.0, 0.0,
                                  0.0, cos(phi), -1.0 * sin(phi), 0.0,
                                  0.0, sin(phi), cos(phi), 0.0,
                                  0.0, 0.0, 0.0, 1.0);

        zNegMat = transpose(zNegMat);
        return zNegMat;
    } else {
        // rotation about z axis
        float zProjDist = sqrt(pow(worldNormal.x, 2) + pow(worldNormal.y, 2));

        float sinThet = -1.f * (worldNormal.y/zProjDist);
        float cosThet = worldNormal.x/zProjDist;

        mat4x4 r1 = mat4x4(cosThet, -1.0 * sinThet, 0.0, 0.0,
                           sinThet, cosThet, 0.0, 0.0,
                           0.0, 0.0, 1.0, 0.0,
                           0.0, 0.0, 0.0, 1.0);
        r1 = transpose(r1);

        // rotation about y axis
        float yProjDist = sqrt(pow(worldNormal.x, 2) +
                               pow(worldNormal.y, 2) +
                               pow(worldNormal.z, 2));

        float sinPhi = -1.f * ((sqrt(pow(worldNormal.x, 2) + pow(worldNormal.y, 2)))/yProjDist);
        float cosPhi = (worldNormal.z)/yProjDist;

        mat4x4 r2 = mat4x4(cosPhi, 0.0, sinPhi, 0.0,
                           0.0, 1.0, 0.0, 0.0,
                           -1.0 * sinPhi, 0.0, cosPhi, 0.0,
                           0.0, 0.0, 0.0, 1.0);
        r2 = transpose(r2);

        return (r2 * r1);
    }
}

// given an intersected object, the eye point and direction of the original ray casted, and
// the normal of the object at the point of intersection, finds the AO contribution
float getAOcontribution(vec4 worldSpacePoint, vec4 worldSpaceDir, vec2 randomSeed)
{
    PrimitiveType intersectObject = getIntersection(worldSpacePoint, worldSpaceDir);

    vec4 worldNormal = getWorldSpaceNormal(intersectObject, worldSpacePoint, worldSpaceDir);
    if (settings.useNM == 1 && intersectObject.blend > 0.0){
        worldNormal = getNormalMappedNormal(intersectObject, worldNormal, worldSpacePoint, worldSpaceDir);
    }

    // find worldSpace Intersection point
    vec4 worldSpaceIntersectionPt = worldSpacePoint + (intersectObject.t * worldSpaceDir);

    // raise the intersection point by epsilon
    vec4 raisedIntersectionPt = worldSpaceIntersectionPt + (SHAPE_EPSILON/2.f * worldNormal);

    // find the matrix that will transform a vector to align with the z axis
    mat4x4 worldToZaxis = getZaxisAlignmentRotation(worldNormal);

    // pre-sets for AO results
    float maxDist = 1.5;

    int sampleNum = 5;
    if (settings.useStochastic == 0) {
        sampleNum = settings.numSamples;
    }

    float sampleNumFloat = float(sampleNum);
    float intersectionCount = 0.0;

    // generate random value for generating samples
    float randVal = randValue2(randomSeed.x, randomSeed.y);

    for (int i = 0; i < sampleNum; i++) {

        // random theta s.t. 0 < theta < pi/2 ( rotating z+ normal away from z axis)
        float maxTheta = PI/2.0 /*- SHAPE_EPSILON*/;
        float theta = randValue2(i, randVal) * maxTheta;

        // random phi s.t. 0 < theta < 2*pi (rotating about z axis
        float maxPhi = (2.0 * PI);
        float phi = randValue2(i, randVal) * maxPhi;

        float newX = cos(phi)/cos(theta);
        float newY = sin(phi)/cos(theta);
        float newZ = sin(theta);

        vec4 hemisphereVec = vec4(newX, newY, newZ, 0.0);
        hemisphereVec = normalize(hemisphereVec);

        vec4 transformedSamplerVec = inverse(worldToZaxis) * hemisphereVec;
        transformedSamplerVec = normalize(transformedSamplerVec);

        PrimitiveType sampledObj = getIntersection(raisedIntersectionPt, transformedSamplerVec);

        if (sampledObj.t > 0) {
            float dotProd = dot(transformedSamplerVec, worldNormal);

            float clampedT = clamp(sampledObj.t, 0.0, maxDist);

            float scaledT = (1.0 - (clampedT/maxDist));

            // [alternate option to scale by dotProduct]
            // scaledT = scaledT * dotProd;

            intersectionCount += scaledT;
        }
    }

    float normalizedIntersectionCount = 1.0 - (intersectionCount/sampleNumFloat);

    return normalizedIntersectionCount;

}

// assuming an intersection of worldSpacePoint/Dir and intersectObject, finds
// necessary normals and calculates color using 'calculateLighting' based on those values.
vec4 getColor(PrimitiveType intersectObject, vec4 worldSpacePoint, vec4 worldSpaceDir)
{
    vec4 worldNormal = getWorldSpaceNormal(intersectObject, worldSpacePoint, worldSpaceDir);
    return calculateLighting(worldNormal, worldSpacePoint, worldSpaceDir, intersectObject);
}

vec4 recursiveRayTrace(vec4 worldSpacePoint, vec4 worldSpaceDir)
{
    vec4 backgroundColor = vec4(0.8, 0.8, 0.8, 1.0);

    if (settings.useEnvironment == 1){
        backgroundColor = vec4(texture(envMap, vec3(worldSpaceDir)).bgr, 1.0);
    }
    // intersected object info for this recursive iteration

    // incoming rays on first iteration are eye point + direction of start ray.
    vec4 worldSpaceIncomingPt = worldSpacePoint;
    vec4 worldSpaceIncomingDir = worldSpaceDir;

    // cumulative colors for each channel. start at 0, will grow at each iteration.
    float cumR = 0.0;
    float cumG = 0.0;
    float cumB = 0.0;

    // reflection scalars for each channel, will be lessened at each iteration.
    float curRscalar = 1.f;
    float curGscalar = 1.f;
    float curBscalar = 1.f;

    float isReflecting = 0.f;

    // calculate reflected ray path!!
    for (int i = 0; i < MAX_BOUNCE; i++) {

        PrimitiveType intersectedObj = getIntersection(worldSpaceIncomingPt,
                                                    worldSpaceIncomingDir);

        if (intersectedObj.t > 0) { // object intersection for current incoming ray

            isReflecting = 1.0;

            // get color calculation for intersected obj
            vec4 color = getColor(intersectedObj,
                                  worldSpaceIncomingPt, worldSpaceIncomingDir);

            // add color of intesected object by our current scalar value;
            cumR += (curRscalar * color.x);
            cumG += (curGscalar * color.y);
            cumB += (curBscalar * color.z);

            // get current object's reflective scalars
            float redReflScalar = intersectedObj.cReflective.x * globalData.ks;
            float greenReflScalar = intersectedObj.cReflective.y * globalData.ks;
            float blueReflScalar = intersectedObj.cReflective.z * globalData.ks;

            // update our current scalars by current color
            curRscalar = curRscalar * redReflScalar;
            curGscalar = curGscalar * greenReflScalar;
            curBscalar = curBscalar * blueReflScalar;

            // update incoming pt + dir:

            // reflectedRay starts at obj's intersection point. get it in world space.
            vec4 reflectedRayStart = getWorldSpaceIntersectionPt(intersectedObj,
                                                                 worldSpaceIncomingPt,
                                                                 worldSpaceIncomingDir);

            // reflectedRay is reflected across object's normal. get normal in world space.
            vec4 worldSpaceNorm = getWorldSpaceNormal(intersectedObj,
                                                      worldSpaceIncomingPt,
                                                      worldSpaceIncomingDir);
            // raise ray start by epsilon along normal:
            vec4 reflectedRayStartRaised = reflectedRayStart + (SHAPE_EPSILON * worldSpaceNorm);

            // get the incoming ray (sight line) in worldSpace
            vec4 incomingRay = normalize(reflectedRayStart - worldSpaceIncomingPt);

            //eye line reflected about object's normal at point of intersection
            vec4 reflectedRayDirection = reflect(incomingRay, worldSpaceNorm);
            reflectedRayDirection = normalize(reflectedRayDirection);

            // update the incoming rays for the next iteration
            worldSpaceIncomingPt = reflectedRayStartRaised;
            worldSpaceIncomingDir = reflectedRayDirection;

        } else {
            // no intersection, terminate iteration
            // if environment cube on, sample the reflection on the skybox
            vec4 refColor = backgroundColor;
            if (settings.useEnvironment == 1 && isReflecting == 1.0){
                refColor = vec4(texture(envMap, vec3(worldSpaceIncomingDir).bgr, 1.0));
            }

            cumR += curRscalar * refColor.x;
            cumG += curGscalar * refColor.y;
            cumB += curBscalar * refColor.z;
            break;
        }
    }

    return vec4(clamp(cumR, 0.0, 1.0),
                clamp(cumG, 0.0, 1.0),
                clamp(cumB, 0.0, 1.0), 1.0);
}

// shootRay: Iterate through objects in scene and check for intersections
// Returns a vec4 (r, g, b, a);
// This takes in vec4 point and vec4 direction in WORLD SPACE
// And per object converts into objectspace
vec4 shootRay(vec4 worldSpacePoint, vec4 worldSpaceDir, vec2 randomSeed){

    // default background color is a light gray
    vec4 outColor = vec4(0.8, 0.8, 0.8, 1.0);
    if (settings.useEnvironment == 1){
        outColor = vec4(texture(envMap, vec3(worldSpaceDir)).bgr, 1.0);
    }
    if (settings.useReflections == 1) {
        outColor = recursiveRayTrace(worldSpacePoint, worldSpaceDir);
    } else {
        // get the closest intersected object with helper method
        PrimitiveType intersectObject = getIntersection(worldSpacePoint, worldSpaceDir);

        // If primitive has an intersection, calculate lighting
        if (intersectObject.primitive != NO_INTERSECT){
            outColor = getColor(intersectObject, worldSpacePoint, worldSpaceDir);
        }
    }

    if (settings.useAO == 1) {
        float aoContribution = getAOcontribution(worldSpacePoint, worldSpaceDir, randomSeed);

        // If no lighting features are enabled, make the color _just_ AO
        if (settings.useAmbient == 0 &&
                settings.useDiffuse == 0 &&
                settings.useSpecular == 0 &&
                settings.useReflections == 0) {
            outColor = vec4(aoContribution, aoContribution, aoContribution, 1.0);
        } else {

            // If lighting features are enabled, multiply in the AO
            outColor = aoContribution * outColor;
        }
    }
    return outColor;
}

// rayTrace:
// Given a camera space eye point and a camera space film point
// Begin the ray tracing process by either shooting from a locally
// randomized eye point (DOF) or the regular eye point.
vec4 rayTrace(vec4 cameraSpaceEye, vec4 cameraSpaceFilmPoint){
    vec4 eye;
    vec4 filmPoint;
    vec4 outColor = vec4(0.0,0.0,0.0,1.0);

    if (settings.useDOF == 1) {
        int numSamples = 1;
        if (settings.useStochastic == 0) {
            numSamples = settings.numSamples;
        }
        for (int i = 0; i < numSamples; i++) {
            // With depth of field. Jitter the eye and move the film point to the focal distance.

            // This vectors can be multiplied by x where -1 <= x <= 1
            // and added to the camera space eye point
            vec4 xDelta = vec4(float(settings.aperture)/50000.0f, 0.0, 0.0, 0.0);
            vec4 yDelta = vec4(0.0, float(settings.aperture)/50000.0f, 0.0, 0.0);

            float randX = randValue2(i, cameraSpaceFilmPoint.x);// * 2 -1.0;
            float randY = randValue2(i, cameraSpaceFilmPoint.y);// * 2 - 1.0;

            eye = inverseCam * (cameraSpaceEye + randX * xDelta + randY * yDelta);
            filmPoint = inverseCam * (vec4(vec3(cameraSpaceFilmPoint) * (float(settings.focalLength)/100.0), 1));
            vec4 rayDirection = filmPoint - eye;
            outColor = outColor + shootRay(eye, rayDirection, filmPoint.xy);
        }
        outColor = vec4(vec3(outColor) / float(numSamples), 1.0);

    } else {
        // No depth of field. Regular eye and film point.
        eye = inverseCam * cameraSpaceEye;
        filmPoint = inverseCam * cameraSpaceFilmPoint;
        vec4 rayDirection = filmPoint - eye;
        outColor = shootRay(eye, rayDirection, filmPoint.xy);
    }

    return outColor;
}

// Assemble scene objects and light lists from uniforms
// FBO pipeline
void main(){

    // initialize scene list
    sceneObjects[0] = sceneObject1;
    sceneObjects[1] = sceneObject2;
    sceneObjects[2] = sceneObject3;
    sceneObjects[3] = sceneObject4;
    sceneObjects[4] = sceneObject5;
    sceneObjects[5] = sceneObject6;

    // initialize light list
    sceneLights[0] = lightObject1;
    sceneLights[1] = lightObject2;
    sceneLights[2] = lightObject3;

    // Use lightIntensitySetting from UI (passed in from view)
    sceneLights[0].lightIntensitySetting = settings.l1Intensity;
    sceneLights[1].lightIntensitySetting = settings.l2Intensity;
    sceneLights[2].lightIntensitySetting = settings.l3Intensity;

    float width = dimensions[0];
    float height = dimensions[1];

    float xFragCoord = gl_FragCoord[0];
    float yFragCoord = gl_FragCoord[1];

    // Scale frag coord to canonical view volume film plane
    // Positioned at z = -1.0
    float u = xFragCoord / width;
    float v = yFragCoord / height;
    u = (u*2.0) - 1.0;
    v = (v*2.0) - 1.0;
    u = u * width/height; // Scale uv.x by screenwidth/screenheight
    // u and v now represent a point on the film plane in world space

    // If using stochastic, instead of sampling in the center of the fragCoord 'pixel'
    // Randomly sample by an offset within this 'pixel'
    if (settings.useStochastic == 1){
        float sizeAcross = 2.f/width;
        float sizeDown = 2.f/height; // size of a 'pixel' on the fragQuad
        // jitter the ray origin from the center of this pixel to within the pixel bounds
        float randU = randValue2(u, time) * 2 -1.0; // scale rand value to [-1.0, 1.0]
        float randV = randValue2(v, time) * 2 -1.0;
        float horizontalRandom = randU * (sizeAcross/2); // Scale by half the size of the frag 'pixel'
        float verticalRandom = randV * (sizeDown/2);
        u = u + horizontalRandom;
        v = v + verticalRandom;
    }

    vec4 filmPoint = vec4(u, v, -1.0, 1.0);
    vec4 eye = vec4(0.0, 0.0, 0.0, 1.0);

    // Begin raytracing
    vec4 currColor = rayTrace(eye, filmPoint);
    vec4 nextColor = currColor;

    // If not first pass, sample from previous pass
    // and scale according to numPasses
    if (settings.useStochastic == 1){
        // Sample prev FBO for this FBO
        float contribution = 1.0/(numPasses+1);
        nextColor =  mix(texture(prev, uv), currColor, contribution);
    }
    fragColor = nextColor;
}
