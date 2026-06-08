#pragma once
#include "Pondo/Core.h"
#include "Pondo/Renderer/Shader.h"
#include <string>
#include <memory>

namespace Pondo
{
    class Scene;

    class PONDO_API SceneSerializer
    {
    public:
        static bool Save(Scene* scene, const std::string& path);
        static bool Load(Scene* scene, const std::string& path, const std::shared_ptr<Shader>& shader);
    };
}