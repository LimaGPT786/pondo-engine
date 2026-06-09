#pragma once

#include <Pondo.h>
#include "Pondo/Scene/Entity.h"
#include <type_traits>
#include <vector>

// -------------------------------------------------------
//  Command interface
// -------------------------------------------------------

class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void Undo() = 0;
    virtual void Redo() = 0;
};

// -------------------------------------------------------
//  TransformCommand
//  GetTransform() returns TransformComponent& so we must
//  strip the reference before using it as a stored member.
// -------------------------------------------------------

class TransformCommand : public ICommand
{
public:
    TransformCommand(
        Pondo::Entity* entity,
        const Pondo::TransformComponent& before,
        const Pondo::TransformComponent& after)
        : m_Entity(entity), m_Before(before), m_After(after)
    {
    }

    void Undo() override { if (m_Entity) m_Entity->GetTransform() = m_Before; }
    void Redo() override { if (m_Entity) m_Entity->GetTransform() = m_After; }

private:
    Pondo::Entity* m_Entity;
    Pondo::TransformComponent m_Before;
    Pondo::TransformComponent m_After;
};

// -------------------------------------------------------
//  MultiTransformCommand
//  Records before/after transforms for a group of entities
//  (used when a gizmo drag moves the whole selection).
// -------------------------------------------------------

struct EntityTransformSnapshot
{
    Pondo::Entity* Entity = nullptr;
    Pondo::TransformComponent Before;
    Pondo::TransformComponent After;
};

class MultiTransformCommand : public ICommand
{
public:
    explicit MultiTransformCommand(std::vector<EntityTransformSnapshot> snapshots)
        : m_Snapshots(std::move(snapshots)) {
    }

    void Undo() override
    {
        for (auto& s : m_Snapshots)
            if (s.Entity) s.Entity->GetTransform() = s.Before;
    }
    void Redo() override
    {
        for (auto& s : m_Snapshots)
            if (s.Entity) s.Entity->GetTransform() = s.After;
    }

private:
    std::vector<EntityTransformSnapshot> m_Snapshots;
};