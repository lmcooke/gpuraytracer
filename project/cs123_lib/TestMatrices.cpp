#include "TestMatrices.h"

#include <iostream>
#include "glm/ext.hpp"

#include "glm/glm.hpp"

TestMatrices::TestMatrices()
{
}


void TestMatrices::test1()
{
    std::cout << "HERE" << std::endl;


    glm::vec4 normal1 = glm::vec4(.25, -1.f, .3, 0.f);
    normal1 = glm::normalize(normal1);

    // rotation about z axis
//    float zProjDist = sqrt(pow(normal1.x, 2) + (normal1.y, 2));

//    float sinThet = -1.f * (normal1.y/zProjDist);
//    float cosThet = normal1.x/zProjDist;

    float zProjDist = sqrt(pow(normal1.x, 2) + pow(normal1.y, 2));

    float sinThet = -1.f * (normal1.y/zProjDist);
    float cosThet = normal1.x/zProjDist;

    glm::mat4x4 r1 = glm::mat4x4(cosThet, -1.f * sinThet, 0.f, 0.f,
                                  sinThet, cosThet, 0.f, 0.f,
                                  0.f, 0.f, 1.f, 0.f,
                                  0.f, 0.f, 0.f, 1.f);
    r1 = glm::transpose(r1);

    glm::vec4 rotatedNorm = r1 * normal1;


    std::cout << "after first rotation" << glm::to_string(rotatedNorm) << std::endl;
    std::cout << "dist : " << glm::length(rotatedNorm) << std::endl;

    // rotation about y axis
    float yProjDist = sqrt(pow(normal1.x, 2) + pow(normal1.z, 2) + pow(normal1.y, 2));

    float sinPhi = -1.f * ((sqrt(pow(normal1.x, 2) + pow(normal1.y, 2)))/yProjDist);
//    float sinPhi = -1.f * (normal1.y/yProjDist);
    float cosPhi = (normal1.z)/yProjDist;

    glm::mat4x4 r2 = glm::mat4x4(cosPhi, 0.f, sinPhi, 0.f,
                                 0.f, 1.f, 0.f, 0.f,
                                 -1 * sinPhi, 0.f, cosPhi, 0.f,
                                 0.f, 0.f, 0.f, 1.f);
    r2 = glm::transpose(r2);

    glm::vec4 finalRotatedNorm = (r2 * r1) * normal1;

    std::cout << "final vec : " << glm::to_string(finalRotatedNorm) << std::endl;

    float pi = 3.1415;

//    float theta = .4 * pi;// 0 < theta < pi/2
//    float phi = .1 * pi; // 0 < phi < 2*pi
//    float len = sqrt(pow(std::sin(theta), 2) + pow(std::cos(theta), 2));

//    float newX = std::cos(phi)/std::cos(theta);
//    float newY = std::sin(phi)/std::cos(theta);
//    float newZ = std::sin(theta);

//    std::cout << "newX : " << newX << std::endl;
//    std::cout << "newY : " << newY << std::endl;
//    std::cout << "newZ : " << newZ << std::endl;

//    glm::vec3 randoVec = glm::vec3(newX, newY, newZ);
//    std::cout << "length : " << glm::length(randoVec);

    for (int i = 0; i < 10; i++) {
        float theta = static_cast<float>(rand())/(RAND_MAX/(pi/2.f));
        float phi = static_cast<float>(rand())/(RAND_MAX/(2.f * pi));

        std::cout << "theta : " << theta << std::endl;
        std::cout << "phi : " << phi << std::endl;

        float newX = std::cos(phi)/std::cos(theta);
        float newY = std::sin(phi)/std::cos(theta);
        float newZ = std::sin(theta);

        std::cout << "newX : " << newX << std::endl;
        std::cout << "newY : " << newY << std::endl;
        std::cout << "newZ : " << newZ << std::endl;

        glm::vec3 randoVec = glm::vec3(newX, newY, newZ);
        std::cout << "length : " << glm::length(randoVec);

        std::cout << "===========" << std::endl;
    }


}
