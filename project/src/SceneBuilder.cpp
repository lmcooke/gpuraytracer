#include "SceneBuilder.h"

#include <iostream>

#include "settings.h"

/**
  [SCENE BUILDER]
  Static library that builds scenes to pass into ray via view.cpp
**/
SceneBuilder::SceneBuilder()
{
}

std::vector<SceneObject> SceneBuilder::getScene(float time)
{
    if (settings.modeScene == 0) {
        return SceneBuilder::buildScene0(time);
    } else if (settings.modeScene == 1) {
        return SceneBuilder::buildScene1(time);
    } else {
        return SceneBuilder::buildScene2(time);
    }
}

std::vector<SceneObject> SceneBuilder::buildScene0(float time)
{
    // Scene Object 1
    float yTranslate = 2.5 * sin(time);
    glm::mat4x4 sceneObject_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 0.5,
                                                             0.0, 1.0, 0.0, -0.5,
                                                             0.0, 0.0, 1.0, -1.5,
                                                             0.0, 0.0, 0.0, 1.f));
    sceneObject_ctm = glm::translate(sceneObject_ctm, glm::vec3(0, yTranslate, 0));

    SceneObject sceneObject1 = {ShapeType::SPHERE, sceneObject_ctm,
                               glm::vec4(0.0, 0.2, 0.7, 1.0),
                               glm::vec4(0.3, 0.3, 0.3, 1.0),
                               glm::vec4(0.0, 0.2, 0.5, 1.0),
                               glm::vec4(0.0, 0.0, 0.5, 1.0), 100.0,
                               0.0, 0,
                               1.0, 1.0};

    // Scene Object 2
    float yTranslate2 = 2.5 * sin(time - 2);
    glm::mat4x4 sceneObject2_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 2.0,
                                                              0.0, 1.0, 0.0, -2.0,
                                                              0.0, 0.0, 1.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    sceneObject2_ctm = glm::translate(sceneObject2_ctm, glm::vec3(0, yTranslate2, 0));
    SceneObject sceneObject2 = {ShapeType::CONE, sceneObject2_ctm,
                               glm::vec4(0.0, 0.5, 0.0, 1.0),
                               glm::vec4(0.6, 0.3, 0.3, 1.0),
                               glm::vec4(0.5, 1.0, 0.0, 1.0),
                               glm::vec4(.5, 0.0, 0.0, 1.0), 15.0,
                               1.0, 0,
                               1.0, 1.0};

    // Scene Object 3
    float yTranslate3 = 2.5 * sin(time - 4);
    glm::mat4x4 sceneObject3_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 3.5,
                                                              0.0, 1.0, 0.0, 1.0,
                                                              0.0, 0.0, 1.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    sceneObject3_ctm = glm::translate(sceneObject3_ctm, glm::vec3(0, yTranslate3, 0));
    SceneObject sceneObject3 = {ShapeType::CYLINDER, sceneObject3_ctm,
                               glm::vec4(1.0, 0.0, 0.1, 1.0),
                               glm::vec4(0.3, 0.0, 0.3, 1.0),
                               glm::vec4(0.5, 0.2, 0.8, 1.0),
                               glm::vec4(.5, 0.0, 0.0, 1.0), 15.0,
                               1.0, 1,
                               1.0, 1.0};

    // Scene Object 4
    float yTranslate4 = 2.5 * sin(time + 2);
    glm::mat4x4 sceneObject4_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, -1.0,
                                                              0.0, 1.0, 0.0, -1.0,
                                                              0.0, 0.0, 1.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    sceneObject4_ctm = glm::translate(sceneObject4_ctm, glm::vec3(0, 0, yTranslate4));
    SceneObject sceneObject4 = {ShapeType::CUBE, sceneObject4_ctm,
                               glm::vec4(0.8, 0.3, 0.1, 1.0),
                               glm::vec4(0.2, 0.3, 0.3, 1.0),
                               glm::vec4(0.1, 1.0, 0.4, 1.0),
                               glm::vec4(.5, 0.0, 0.0, 1.0), 15.0,
                               1.0, 0,
                               1.0, 1.0};

    // Scene Object 5
    float yTranslate5 = 2.5 * sin(time + 4);
    glm::mat4x4 sceneObject5_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, -2.5,
                                                              0.0, 1.0, 0.0, 0.5,
                                                              0.0, 0.0, 1.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    sceneObject5_ctm = glm::translate(sceneObject5_ctm, glm::vec3(0, yTranslate5, 0));
    SceneObject sceneObject5 = {ShapeType::SPHERE, sceneObject5_ctm,
                               glm::vec4(0.3, 0.3, 0.5, 1.0),
                               glm::vec4(0.1, 0.5, 0.2, 1.0),
                               glm::vec4(0.7, .9, 0.8, 1.0),
                               glm::vec4(.5, 0.0, 0.0, 1.0), 15.0,
                               1.0, 2,
                               1.0, 1.0};

    // Scene Object 6
    float yTranslate6 = 2.5 * sin(time + 6);
    glm::mat4x4 sceneObject6_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, -4.0,
                                                              0.0, 1.0, 0.0, 0.0,
                                                              0.0, 0.0, 1.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    sceneObject6_ctm = glm::translate(sceneObject6_ctm, glm::vec3(0, 0, yTranslate6));
    SceneObject sceneObject6 = {ShapeType::CYLINDER, sceneObject6_ctm,
                               glm::vec4(0.6, 0.3, 0.5, 1.0),
                               glm::vec4(0.4, 0.4, 0.3, 1.0),
                               glm::vec4(0.7, .1, 0.8, 1.0),
                               glm::vec4(.5, 0.7, 0.3, 1.0), 15.0,
                               1.0, 0,
                               1.0, 1.0};

    std::vector<SceneObject> sceneObjects;
    sceneObjects.reserve(6);

    sceneObjects.push_back(sceneObject1);
    sceneObjects.push_back(sceneObject2);
    sceneObjects.push_back(sceneObject3);
    sceneObjects.push_back(sceneObject4);
    sceneObjects.push_back(sceneObject5);
    sceneObjects.push_back(sceneObject6);

    return sceneObjects;

}

std::vector<SceneObject> SceneBuilder::buildScene1(float time)
{
    // Scene Object 1
    glm::vec4 intrsctSphereDiff = glm::vec4(1.f, 1.f, 1.f, 1.f);
    glm::vec4 intrsctSphereAmbient = glm::vec4(.9, .9, .9, 1.f);
    glm::vec4 intrsctSphereSpec = intrsctSphereDiff;
    glm::vec4 intrsctSphereRefl = intrsctSphereDiff;

    glm::mat4x4 sceneObject_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 0.0,
                                                             0.0, 1.0, 0.0, 0.0,
                                                             0.0, 0.0, 1.0, -0.5,
                                                             0.0, 0.0, 0.0, 1.f));

    SceneObject sceneObject1 = {ShapeType::SPHERE, sceneObject_ctm,
                               intrsctSphereDiff,
                               intrsctSphereAmbient,
                               intrsctSphereSpec, // spec
                               intrsctSphereRefl, 100.0,
                               0.0, 0,
                               1.0, 1.0};

    // Scene Object 2 : BASE
    glm::vec4 baseDiff = glm::vec4(.84, .99, 1.f, 1.f);
    glm::vec4 baseAmbient = glm::vec4(.2, .2, .2, 1.f);
    glm::vec4 baseSpec = baseDiff;
    glm::vec4 baseRefl = baseDiff;

    glm::mat4x4 sceneObject2_ctm = glm::transpose(glm::mat4x4(6.0, 0.0, 0.0, 0.0,
                                                              0.0, 0.15, 0.0, 0.55,
                                                              0.0, 0.0, 6.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    float phi = 3.1415;
    glm::mat4x4 rotationMat2 = glm::mat4x4(cos(phi), -1.f * sin(phi), 0.f, 0.f,
                                           sin(phi), cos(phi), 0.f, 0.f,
                                           0.f, 0.f, 1.f, 0.f,
                                           0.f, 0.f, 0.f, 1.f);
    sceneObject2_ctm = rotationMat2 * sceneObject2_ctm;
    SceneObject sceneObject2 = {ShapeType::CYLINDER, sceneObject2_ctm,
                               baseDiff,
                               baseAmbient,
                               baseSpec,
                               baseRefl, 15.0,
                               1.0, 1,
                               5.0, 5.0};

    // Scene Object 3
    glm::vec4 cylDiff = glm::vec4(.63, .79, 1.f, 1.f);
    glm::vec4 cylAmbient = glm::vec4(.2, .2, .2, 1.f);
    glm::vec4 cylSpec = cylDiff;
    glm::vec4 cylRefl = cylDiff;

    float yTranslate3 = std::fabs(sin(time + 6));
    glm::mat4x4 sceneObject3_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 2.0,
                                                              0.0, 0.5, 0.0, -0.1,
                                                              0.0, 0.0, 1.0, -2.6,
                                                              0.0, 0.0, 0.0, 1.f));

    sceneObject3_ctm = glm::translate(sceneObject3_ctm, glm::vec3(0, yTranslate3, 0));
    SceneObject sceneObject3 = {ShapeType::CYLINDER, sceneObject3_ctm,
                               cylDiff,
                               cylAmbient,
                               cylSpec,
                               cylRefl, 15.0,
                               1.0, 0,
                               1.0, 1.0};

    // Scene Object 4
    glm::vec4 cubeDiff = glm::vec4(.84, .83, 1.f, 1.f);
    glm::vec4 cubeAmbient = glm::vec4(.2, .2, .2, 1.f);
    glm::vec4 cubeSpec = cubeDiff;
    glm::vec4 cubeRefl = cubeDiff;

    glm::mat4x4 sceneObject4_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, -.5,
                                                              0.0, 1.0, 0.0, 0.0,
                                                              0.0, 0.0, 1.0, -0.5,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject4 = {ShapeType::CUBE, sceneObject4_ctm,
                               cubeDiff,
                               cubeAmbient,
                               cubeSpec,
                               cubeRefl, 15.0,
                               1.0, 2,
                               1.0, 1.0};

    // Scene Object 5: edge sphere
    glm::vec4 edgeSphereDiff = glm::vec4(.89, .71, 1.f, 1.f);
    glm::vec4 edgeSphereAmbient = glm::vec4(.2, .2, .2, 1.f);
    glm::vec4 edgeSphereSpec = edgeSphereDiff;
    glm::vec4 edgeSphereRefl = edgeSphereDiff;

    glm::mat4x4 sceneObject5_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, -3.0,
                                                              0.0, 1.0, 0.0, 0.0,
                                                              0.0, 0.0, 1.0, -1.5,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject5 = {ShapeType::SPHERE, sceneObject5_ctm,
                               edgeSphereDiff,
                               edgeSphereAmbient,
                               edgeSphereSpec,
                               edgeSphereRefl, 11.0,
                               1.0, 0,
                               1.0, 1.0};

    // Scene Object 6 : upside down cone
    glm::vec4 coneDif = glm::vec4(.57, 1.f, .83, 1.f);
    glm::vec4 coneAmbient = glm::vec4(.2, .2, .2, 1.f);
    glm::vec4 coneSpec = coneDif;
    glm::vec4 coneRefl = coneDif;

    glm::mat4x4 sceneObject6_ctm = glm::mat4x4(cos(phi), -1.f * sin(phi), 0.f, 0.f,
                                           sin(phi), cos(phi), 0.f, 0.f,
                                           0.f, 0.f, 1.f, 0.f,
                                           0.f, 0.f, 0.f, 1.f);
    glm::mat4x4 translationMat6 = glm::transpose(glm::mat4x4(2.25, 0.0, 0.0, -1.0,
                                                              0.0, .85, 0.0, -0.2,
                                                              0.0, 0.0, 2.25, -2.0,
                                                              0.0, 0.0, 0.0, 1.f));
    sceneObject6_ctm = translationMat6 * sceneObject6_ctm;


    SceneObject sceneObject6 = {ShapeType::CONE, sceneObject6_ctm,
                               coneDif,
                               coneAmbient,
                               coneSpec,
                               coneRefl, 20.0,
                               1.0, 0,
                               2.0, 2.0};

    std::vector<SceneObject> sceneObjects;
    sceneObjects.reserve(6);

    sceneObjects.push_back(sceneObject1);
    sceneObjects.push_back(sceneObject2);
    sceneObjects.push_back(sceneObject3);
    sceneObjects.push_back(sceneObject4);
    sceneObjects.push_back(sceneObject5);
    sceneObjects.push_back(sceneObject6);

    return sceneObjects;
}

std::vector<SceneObject> SceneBuilder::buildScene2(float time)
{
    // Scene Object 1
    glm::vec4 sphereDif = glm::vec4(.85, .94, .65, 1.f);
    glm::vec4 sphereAmbient = glm::vec4(.1, .1, .05, 1.f);
    glm::vec4 sphereSpec = glm::vec4(1.f, 1.f, 1.f, 1.f);
    glm::vec4 sphereRefl = glm::vec4(.2, .2, .2, 1.f);

    float yTranslate = 3.5 * sin(time);
    glm::mat4x4 sceneObject_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 3.0,
                                                             0.0, 1.0, 0.0, 0.0,
                                                             0.0, 0.0, 1.0, 2.5,
                                                             0.0, 0.0, 0.0, 1.f));
    sceneObject_ctm = glm::translate(sceneObject_ctm, glm::vec3(-2, 0, yTranslate));

    SceneObject sceneObject1 = {ShapeType::SPHERE, sceneObject_ctm,
                               sphereDif,
                               sphereAmbient,
                               sphereSpec,
                               sphereRefl, 15.0,
                               1.0, 0,
                               1.0, 1.0};

    // ------ CYLINDERS ---------

    glm::vec4 cylinderDiffuse1 = glm::vec4(1.0, .95, 0.73, 1.0);
    glm::vec4 cylinderDiffuse2 = glm::vec4(1.0, .92, 0.57, 1.0);
    glm::vec4 cylinderDiffuse3 = glm::vec4(1.0, .89, 0.38, 1.0);
    glm::vec4 cylinderDiffuse4 = glm::vec4(1.0, .85, 0.17, 1.0);



    glm::vec4 cylinderAmbient = glm::vec4(.1, 0.0, 0.0, 1.0);
    glm::vec4 cylinderSpec = glm::vec4(0.3, 0.3, 0.3, 1.f);
    glm::vec4 cylinderRefl = glm::vec4(.2, 0.1, 0.02, 1.0);
    float cylinderShininess = 5.f;
    float cylinderBlend = 1.f;

    int cylinderTexID = 1;

    // Scene Object 2
    glm::mat4x4 sceneObject2_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 3.0,
                                                              0.0, 9.0, 0.0, 2.5,
                                                              0.0, 0.0, 1.0, 5.0,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject2 = {ShapeType::CYLINDER, sceneObject2_ctm,
                               cylinderDiffuse1,
                               cylinderAmbient,
                               cylinderSpec,
                               cylinderRefl, cylinderShininess,
                               cylinderBlend, cylinderTexID,
                               2.0, 2.0};

    // Scene Object 3
    glm::mat4x4 sceneObject3_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 3.0,
                                                              0.0, 8.0, 0.0, 2.0,
                                                              0.0, 0.0, 1.0, 2.0,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject3 = {ShapeType::CYLINDER, sceneObject3_ctm,
                                cylinderDiffuse2,
                                cylinderAmbient,
                                cylinderSpec,
                                cylinderRefl, cylinderShininess,
                                cylinderBlend, cylinderTexID,
                                2.0, 2.0};

    // Scene Object 4
    glm::mat4x4 sceneObject4_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 3.0,
                                                              0.0, 7.0, 0.0, 1.5,
                                                              0.0, 0.0, 1.0, -1.0,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject4 = {ShapeType::CYLINDER, sceneObject4_ctm,
                                cylinderDiffuse3,
                                cylinderAmbient,
                                cylinderSpec,
                                cylinderRefl, cylinderShininess,
                                cylinderBlend, cylinderTexID,
                                2.0, 2.0};


    // Scene Object 5
    glm::mat4x4 sceneObject5_ctm = glm::transpose(glm::mat4x4(1.0, 0.0, 0.0, 3.0,
                                                              0.0, 6.0, 0.0, 1.0,
                                                              0.0, 0.0, 1.0, -4.0,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject5 = {ShapeType::CYLINDER, sceneObject5_ctm,
                                cylinderDiffuse4,
                                cylinderAmbient,
                                cylinderSpec,
                                cylinderRefl, cylinderShininess,
                                cylinderBlend, cylinderTexID,
                                2.0, 2.0};

    // ---- PLANE ---

    glm::vec4 planeDiffuse = glm::vec4(1.0, .9, .96, 1.0);
    glm::vec4 planeAmbient = glm::vec4(.2, .2, .1, 1.f);
    glm::vec4 planeSpec = glm::vec4(1.f, 1.f, 1.f, 1.f);
    glm::vec4 planeRefl = glm::vec4(0.5, 0.5, 0.5, 1.f);

    // Scene Object 6
    glm::mat4x4 sceneObject6_ctm = glm::transpose(glm::mat4x4(14.0, 0.0, 0.0, -1.5,
                                                              0.0, 0.3, 0.0, -2.0,
                                                              0.0, 0.0, 14.0, 0.5,
                                                              0.0, 0.0, 0.0, 1.f));
    SceneObject sceneObject6 = {ShapeType::CUBE, sceneObject6_ctm,
                               planeDiffuse,
                               planeAmbient,
                               planeSpec,
                               planeRefl, 15.0,
                               1.0, 2, 3.0, 3.0};



    std::vector<SceneObject> sceneObjects;
    sceneObjects.reserve(6);

    sceneObjects.push_back(sceneObject1);
    sceneObjects.push_back(sceneObject2);
    sceneObjects.push_back(sceneObject3);
    sceneObjects.push_back(sceneObject4);
    sceneObjects.push_back(sceneObject5);
    sceneObjects.push_back(sceneObject6);

    return sceneObjects;
}
