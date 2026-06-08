#pragma once

#include <Pondo.h>
#include "Pondo/Scene/Entity.h"
#include <type_traits>

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
