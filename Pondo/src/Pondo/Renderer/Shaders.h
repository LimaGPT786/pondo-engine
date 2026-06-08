#pragma once

// -------------------------------------------------------
//  Built-in GLSL shader sources
//  Lit mesh shader + flat/unlit shader used for gizmos/grid
// -------------------------------------------------------

static const char* s_VertSrc = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec2 a_TexCoord;
    uniform mat4 u_ViewProjection;
    uniform mat4 u_Transform;
    out vec3 v_Normal;
    out vec2 v_TexCoord;
    void main() {
        v_Normal   = a_Normal;
        v_TexCoord = a_TexCoord;
        gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
    }
)";

static const char* s_FragSrc = R"(
    #version 450 core
    in  vec3 v_Normal;
    in  vec2 v_TexCoord;
    out vec4 FragColor;
    uniform vec4 u_Color;
    void main() {
        vec3  lightDir = normalize(vec3(0.8, 1.0, 0.6));
        float diff     = max(dot(normalize(v_Normal), lightDir), 0.0);
        float ambient  = 0.25;
        float lighting = ambient + diff * (1.0 - ambient);
        FragColor = vec4(u_Color.rgb * lighting, u_Color.a);
    }
)";

static const char* s_FlatVertSrc = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    uniform mat4 u_ViewProjection;
    uniform mat4 u_Transform;
    void main() {
        gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
    }
)";

static const char* s_FlatFragSrc = R"(
    #version 450 core
    out vec4 FragColor;
    uniform vec4 u_Color;
    void main() { FragColor = u_Color; }
)";