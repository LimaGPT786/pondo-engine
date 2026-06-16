#include "Pondo/Scene/SceneSerializer.h"
#include "Pondo/Scene/Scene.h"
#include "Pondo/Scene/Entity.h"
#include "Pondo/Renderer/Mesh.h"
#include "Pondo/Renderer/Material.h"
#include "Pondo/Log.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

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

            std::string meshType = "None";
            if (auto* mc = entity->GetMesh(); mc && mc->MeshData)
                meshType = mc->MeshData->GetType();

            glm::vec4 color = { 1, 1, 1, 1 };
            if (auto* mat = entity->GetMaterial(); mat && mat->Mat)
                color = mat->Mat->GetColor();

            file << "BeginEntity\n";
            file << "ID " << entity->GetID() << "\n";   // stable key for group remap on load
            file << "Name " << entity->GetTag() << "\n";
            file << "Mesh " << meshType << "\n";
            file << "Position " << tf.Position.x << " " << tf.Position.y << " " << tf.Position.z << "\n";
            file << "Rotation " << tf.Rotation.x << " " << tf.Rotation.y << " " << tf.Rotation.z << "\n";
            file << "Scale " << tf.Scale.x << " " << tf.Scale.y << " " << tf.Scale.z << "\n";
            file << "Color " << color.r << " " << color.g << " " << color.b << " " << color.a << "\n";

            // Light
            if (auto* lc = entity->GetLight(); lc)
            {
                file << "Light "
                    << (int)lc->Type << " "
                    << (int)lc->Enabled << " "
                    << lc->Color.r << " " << lc->Color.g << " " << lc->Color.b << " "
                    << lc->Intensity << " "
                    << lc->Direction.x << " " << lc->Direction.y << " " << lc->Direction.z << " "
                    << lc->Range << " " << lc->Constant << " " << lc->Linear << " "
                    << lc->Quadratic << " " << lc->InnerCutoffDeg << " " << lc->OuterCutoffDeg << "\n";
            }

            // Script
            if (auto* sc = entity->GetScript(); sc && !sc->ScriptPath.empty())
                file << "Script " << sc->ScriptPath << "\n";

            // Group root
            if (auto* gr = entity->GetGroupRoot(); gr)
                file << "GroupRoot " << (int)gr->Unioned << "\n";

            // Group membership
            if (auto* gc = entity->GetGroup(); gc)
                file << "Group " << gc->ParentID << " " << (int)gc->IsNegate << "\n";

            file << "EndEntity\n\n";
        }

        std::cout << "[Pondo] Scene saved: " << full << "\n";
        return true;
    }

    bool SceneSerializer::Load(Scene* scene, const std::string& path,
        const std::shared_ptr<Shader>& shader)
    {
        if (!scene) { std::cout << "[Pondo] Load: null scene\n";  return false; }
        if (!shader) { std::cout << "[Pondo] Load: null shader\n"; return false; }

        auto full = std::filesystem::absolute(path);
        std::ifstream file(full);
        if (!file) { std::cout << "[Pondo] Could not open: " << full << "\n"; return false; }

        std::string header;
        std::getline(file, header);
        if (header != "PONDO_SCENE") { std::cout << "[Pondo] Bad header\n"; return false; }

        scene->Clear();

        struct RawEntity
        {
            uint32_t    savedID = 0;
            std::string name = "Entity", meshType = "Cube";
            glm::vec3   pos{}, rot{}, scale{ 1, 1, 1 };
            glm::vec4   color{ 1, 1, 1, 1 };
            bool        hasLight = false;
            bool        hasScript = false;
            bool        isGroupRoot = false;
            bool        groupUnioned = false;
            bool        inGroup = false;
            uint32_t    groupParent = 0;
            bool        isNegate = false;
            std::string scriptPath;
            LightComponent light;
        };

        std::vector<RawEntity> raws;
        RawEntity current;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "BeginEntity")
            {
                current = RawEntity{};
            }
            else if (token == "ID") { ss >> current.savedID; }
            else if (token == "Name")
            {
                std::getline(ss, current.name);
                if (!current.name.empty() && current.name[0] == ' ')
                    current.name = current.name.substr(1);
            }
            else if (token == "Mesh") { ss >> current.meshType; }
            else if (token == "Position") { ss >> current.pos.x >> current.pos.y >> current.pos.z; }
            else if (token == "Rotation") { ss >> current.rot.x >> current.rot.y >> current.rot.z; }
            else if (token == "Scale") { ss >> current.scale.x >> current.scale.y >> current.scale.z; }
            else if (token == "Color") { ss >> current.color.r >> current.color.g >> current.color.b >> current.color.a; }
            else if (token == "Light")
            {
                int type, enabled;
                ss >> type >> enabled
                    >> current.light.Color.r >> current.light.Color.g >> current.light.Color.b
                    >> current.light.Intensity
                    >> current.light.Direction.x >> current.light.Direction.y >> current.light.Direction.z
                    >> current.light.Range >> current.light.Constant >> current.light.Linear
                    >> current.light.Quadratic >> current.light.InnerCutoffDeg >> current.light.OuterCutoffDeg;
                current.light.Type = (LightType)type;
                current.light.Enabled = (bool)enabled;
                current.hasLight = true;
            }
            else if (token == "Script")
            {
                std::getline(ss, current.scriptPath);
                if (!current.scriptPath.empty() && current.scriptPath[0] == ' ')
                    current.scriptPath = current.scriptPath.substr(1);
                current.hasScript = true;
            }
            else if (token == "GroupRoot")
            {
                int unioned;
                ss >> unioned;
                current.isGroupRoot = true;
                current.groupUnioned = (bool)unioned;
            }
            else if (token == "Group")
            {
                ss >> current.groupParent >> current.isNegate;
                current.inGroup = true;
            }
            else if (token == "EndEntity")
            {
                raws.push_back(current);
            }
        }

        // Pass 1: create all entities and build savedID -> runtime Entity* map.
        std::unordered_map<uint32_t, Entity*> oldIDToEntity;

        for (auto& raw : raws)
        {
            std::shared_ptr<Mesh> mesh;
            if (raw.meshType == "Sphere")   mesh = Mesh::CreateSphere();
            else if (raw.meshType == "Plane")    mesh = Mesh::CreatePlane(1.0f);
            else if (raw.meshType == "Cylinder") mesh = Mesh::CreateCylinder();
            else if (raw.meshType == "Cube")     mesh = Mesh::CreateCube();
            // "None" = no mesh (group root or light-only entity)

            auto* e = scene->CreateEntity(raw.name);
            auto& tf = e->GetTransform();
            tf.Position = raw.pos;
            tf.Rotation = raw.rot;
            tf.Scale = raw.scale;

            if (mesh)
            {
                e->SetMesh(mesh);
                auto mat = std::make_shared<Material>(shader);
                e->SetMaterial(mat);
                e->GetMaterial()->Mat->SetColor(raw.color);
            }

            if (raw.hasLight)
            {
                e->AddLight(raw.light.Type);
                *e->GetLight() = raw.light;
            }

            if (raw.hasScript)
                e->SetScript(raw.scriptPath);

            if (raw.isGroupRoot)
                e->MakeGroupRoot(raw.name);

            if (raw.savedID != 0)
                oldIDToEntity[raw.savedID] = e;
        }

        // Pass 2: wire up group relationships now that every entity exists and
        // oldIDToEntity maps every saved ID to its new runtime counterpart.
        {
            size_t i = 0;
            for (auto& ep : scene->GetEntities())
            {
                if (i < raws.size() && raws[i].inGroup)
                {
                    auto it = oldIDToEntity.find(raws[i].groupParent);
                    if (it != oldIDToEntity.end())
                        ep->SetGroup(it->second->GetID(), raws[i].isNegate);
                    else
                        PD_CORE_WARN("SceneSerializer: group parent ID {0} not found — re-save the scene to fix", raws[i].groupParent);
                }
                ++i;
            }
        }

        std::cout << "[Pondo] Scene loaded: " << full << "\n";
        return true;
    }
}