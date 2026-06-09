#include "Pondo/Scene/SceneSerializer.h"
#include "Pondo/Scene/Scene.h"
#include "Pondo/Scene/Entity.h"
#include "Pondo/Renderer/Mesh.h"
#include "Pondo/Renderer/Material.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace Pondo
{
    bool SceneSerializer::Save(Scene* scene, const std::string& path)
    {
        if (!scene) return false;

        auto full = std::filesystem::absolute(path);
        std::filesystem::create_directories(full.parent_path());

        std::ofstream file(full);
        if (!file) return false;

        file << "PONDO_SCENE\n\n";

        for (auto& entity : scene->GetEntities())
        {
            auto& tf = entity->GetTransform();

            std::string meshType = "Cube";
            if (auto* mc = entity->GetMesh(); mc && mc->MeshData)
                meshType = mc->MeshData->GetType();

            glm::vec4 color = { 1, 1, 1, 1 };
            if (auto* mat = entity->GetMaterial(); mat && mat->Mat)
                color = mat->Mat->GetColor();

            file << "BeginEntity\n";
            file << "Name "     << entity->GetTag() << "\n";
            file << "Mesh "     << meshType << "\n";
            file << "Position " << tf.Position.x << " " << tf.Position.y << " " << tf.Position.z << "\n";
            file << "Rotation " << tf.Rotation.x << " " << tf.Rotation.y << " " << tf.Rotation.z << "\n";
            file << "Scale "    << tf.Scale.x    << " " << tf.Scale.y    << " " << tf.Scale.z    << "\n";
            file << "Color "    << color.r << " " << color.g << " " << color.b << " " << color.a << "\n";

            // Serialize light component if present
            if (auto* lc = entity->GetLight(); lc)
            {
                file << "Light "
                     << (int)lc->Type      << " "
                     << (int)lc->Enabled   << " "
                     << lc->Color.r        << " "
                     << lc->Color.g        << " "
                     << lc->Color.b        << " "
                     << lc->Intensity      << " "
                     << lc->Direction.x    << " "
                     << lc->Direction.y    << " "
                     << lc->Direction.z    << " "
                     << lc->Range          << " "
                     << lc->Constant       << " "
                     << lc->Linear         << " "
                     << lc->Quadratic      << " "
                     << lc->InnerCutoffDeg << " "
                     << lc->OuterCutoffDeg << "\n";
            }

            file << "EndEntity\n\n";
        }

        std::cout << "[Pondo] Scene saved: " << full << "\n";
        return true;
    }

    bool SceneSerializer::Load(Scene* scene, const std::string& path,
                               const std::shared_ptr<Shader>& shader)
    {
        if (!scene)  { std::cout << "[Pondo] Load: null scene\n";  return false; }
        if (!shader) { std::cout << "[Pondo] Load: null shader\n"; return false; }

        auto full = std::filesystem::absolute(path);
        std::ifstream file(full);
        if (!file) { std::cout << "[Pondo] Could not open: " << full << "\n"; return false; }

        std::string header;
        std::getline(file, header);
        if (header != "PONDO_SCENE") { std::cout << "[Pondo] Bad header\n"; return false; }

        scene->Clear();

        std::string line;
        std::string  name = "Entity", meshType = "Cube";
        glm::vec3    pos{}, rot{}, scale{ 1, 1, 1 };
        glm::vec4    color{ 1, 1, 1, 1 };
        bool         hasLight    = false;
        LightComponent lightData;

        while (std::getline(file, line))
        {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "BeginEntity")
            {
                name = "Entity"; meshType = "Cube";
                pos = {}; rot = {}; scale = { 1, 1, 1 }; color = { 1, 1, 1, 1 };
                hasLight = false;
                lightData = LightComponent{};
            }
            else if (token == "Name")
            {
                std::getline(ss, name);
                if (!name.empty() && name[0] == ' ') name = name.substr(1);
            }
            else if (token == "Mesh")     { ss >> meshType; }
            else if (token == "Position") { ss >> pos.x   >> pos.y   >> pos.z; }
            else if (token == "Rotation") { ss >> rot.x   >> rot.y   >> rot.z; }
            else if (token == "Scale")    { ss >> scale.x >> scale.y >> scale.z; }
            else if (token == "Color")    { ss >> color.r >> color.g >> color.b >> color.a; }
            else if (token == "Light")
            {
                int type, enabled;
                ss >> type >> enabled
                   >> lightData.Color.r     >> lightData.Color.g     >> lightData.Color.b
                   >> lightData.Intensity
                   >> lightData.Direction.x >> lightData.Direction.y >> lightData.Direction.z
                   >> lightData.Range       >> lightData.Constant    >> lightData.Linear
                   >> lightData.Quadratic   >> lightData.InnerCutoffDeg >> lightData.OuterCutoffDeg;
                lightData.Type    = (LightType)type;
                lightData.Enabled = (bool)enabled;
                hasLight = true;
            }
            else if (token == "EndEntity")
            {
                std::shared_ptr<Mesh> mesh;
                if      (meshType == "Sphere")   mesh = Mesh::CreateSphere();
                else if (meshType == "Plane")     mesh = Mesh::CreatePlane(1.0f);
                else if (meshType == "Cylinder")  mesh = Mesh::CreateCylinder();
                else                              mesh = Mesh::CreateCube();

                auto* e  = scene->CreateEntity(name);
                auto& tf = e->GetTransform();
                tf.Position = pos;
                tf.Rotation = rot;
                tf.Scale    = scale;
                e->SetMesh(mesh);

                auto mat = std::make_shared<Material>(shader);
                e->SetMaterial(mat);
                e->GetMaterial()->Mat->SetColor(color);

                if (hasLight)
                {
                    e->AddLight(lightData.Type);
                    *e->GetLight() = lightData;
                }
            }
        }

        std::cout << "[Pondo] Scene loaded: " << full << "\n";
        return true;
    }
}
