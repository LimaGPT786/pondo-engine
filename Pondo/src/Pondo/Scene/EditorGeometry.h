#pragma once

#include <Pondo.h>
#include <glad/glad.h>
#include <memory>
#include <vector>

// -------------------------------------------------------
//  Grid geometry
// -------------------------------------------------------

struct GridRenderer {
    std::shared_ptr<Pondo::VertexArray> VA;
    int lineCount = 0;

    void Build(int halfSize, int step) {
        std::vector<float> v;
        for (int i = -halfSize; i <= halfSize; i += step) {
            float f = (float)i;
            v.insert(v.end(), { f,0,-(float)halfSize,  f,0,(float)halfSize });
            v.insert(v.end(), { -(float)halfSize,0,f,  (float)halfSize,0,f });
        }
        lineCount = (int)v.size() / 3;
        auto vb = std::make_shared<Pondo::VertexBuffer>(v.data(), (unsigned int)(v.size() * sizeof(float)));
        VA = std::make_shared<Pondo::VertexArray>();
        VA->AddVertexBuffer(vb);
    }

    void Draw() const {
        VA->Bind();
        glDrawArrays(GL_LINES, 0, lineCount);
        VA->Unbind();
    }
};

// -------------------------------------------------------
//  Arrow gizmo geometry
// -------------------------------------------------------

struct ArrowGizmo {
    std::shared_ptr<Pondo::VertexArray> VA;
    int vertCount = 0;

    void Build() {
        float s = 0.07f, tip = 1.0f, base = 0.78f;
        std::vector<float> v = {
            0,0,0,   0,tip,0,
            0,tip,0,  s,base,0,    0,tip,0, -s,base,0,
            0,tip,0,  0,base,s,    0,tip,0,  0,base,-s,
        };
        vertCount = (int)v.size() / 3;
        auto vb = std::make_shared<Pondo::VertexBuffer>(v.data(), (unsigned int)(v.size() * sizeof(float)));
        VA = std::make_shared<Pondo::VertexArray>();
        VA->AddVertexBuffer(vb);
    }

    void Draw() const {
        VA->Bind();
        glDrawArrays(GL_LINES, 0, vertCount);
        VA->Unbind();
    }
};