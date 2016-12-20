#ifndef SCENEBUILDER_H
#define SCENEBUILDER_H

#include "view.h"


class SceneBuilder
{
public:
    SceneBuilder();

    static std::vector<SceneObject> getScene(float time);

    static std::vector<SceneObject> buildScene0(float time);

    static std::vector<SceneObject> buildScene1(float time);

    static std::vector<SceneObject> buildScene2(float time);

};

#endif // SCENEBUILDER_H
