#pragma once

// -------------------------------------------------------
//  Built-in GLSL shader sources
//  Lit mesh shader — supports:
//    - Ambient light
//    - One directional (sun) light
//    - Up to 8 point lights
//    - Up to 4 spot lights
//  Flat/unlit shader used for gizmos and the grid.
// -------------------------------------------------------

static const char* s_VertSrc = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec2 a_TexCoord;

    uniform mat4 u_ViewProjection;
    uniform mat4 u_Transform;

    out vec3 v_FragPos;
    out vec3 v_Normal;
    out vec2 v_TexCoord;

    void main()
    {
        vec4 worldPos   = u_Transform * vec4(a_Position, 1.0);
        v_FragPos       = worldPos.xyz;
        // Use the normal matrix to handle non-uniform scaling correctly
        v_Normal        = mat3(transpose(inverse(u_Transform))) * a_Normal;
        v_TexCoord      = a_TexCoord;
        gl_Position     = u_ViewProjection * worldPos;
    }
)";

static const char* s_FragSrc = R"(
    #version 450 core

    in  vec3 v_FragPos;
    in  vec3 v_Normal;
    in  vec2 v_TexCoord;
    out vec4 FragColor;

    // ---- material ----
    uniform vec4 u_Color;

    // ---- camera ----
    uniform vec3 u_CameraPos;

    // ---- ambient ----
    uniform vec3  u_AmbientColor;
    uniform float u_AmbientIntensity;

    // ---- sun (directional) ----
    uniform int   u_SunActive;
    uniform vec3  u_SunDirection;
    uniform vec3  u_SunColor;
    uniform float u_SunIntensity;

    // ---- point lights ----
    #define MAX_POINT_LIGHTS 8
    uniform int   u_PointLightCount;
    uniform vec3  u_PointLightPos[MAX_POINT_LIGHTS];
    uniform vec3  u_PointLightColor[MAX_POINT_LIGHTS];
    uniform float u_PointLightIntensity[MAX_POINT_LIGHTS];
    uniform float u_PointLightRange[MAX_POINT_LIGHTS];

    // ---- spot lights ----
    #define MAX_SPOT_LIGHTS 4
    uniform int   u_SpotLightCount;
    uniform vec3  u_SpotLightPos[MAX_SPOT_LIGHTS];
    uniform vec3  u_SpotLightDir[MAX_SPOT_LIGHTS];
    uniform vec3  u_SpotLightColor[MAX_SPOT_LIGHTS];
    uniform float u_SpotLightIntensity[MAX_SPOT_LIGHTS];
    uniform float u_SpotLightInnerCos[MAX_SPOT_LIGHTS];
    uniform float u_SpotLightOuterCos[MAX_SPOT_LIGHTS];

    // -------------------------------------------------------
    //  Blinn-Phong helpers
    // -------------------------------------------------------

    vec3 CalcDiffuseSpecular(vec3 lightDir, vec3 normal, vec3 viewDir,
                             vec3 lightColor, float intensity)
    {
        float diff = max(dot(normal, lightDir), 0.0);

        vec3  halfDir = normalize(lightDir + viewDir);
        float spec    = pow(max(dot(normal, halfDir), 0.0), 32.0);

        return (diff + spec * 0.4) * lightColor * intensity;
    }

    float CalcAttenuation(float dist, float constant, float linear, float quadratic)
    {
        return 1.0 / (constant + linear * dist + quadratic * dist * dist);
    }

    // -------------------------------------------------------
    //  Main
    // -------------------------------------------------------
    void main()
    {
        vec3  normal  = normalize(v_Normal);
        vec3  viewDir = normalize(u_CameraPos - v_FragPos);
        vec3  result  = vec3(0.0);

        // -- ambient --
        result += u_AmbientColor * u_AmbientIntensity;

        // -- sun --
        if (u_SunActive != 0)
        {
            vec3 sunDir = normalize(-u_SunDirection);
            result += CalcDiffuseSpecular(sunDir, normal, viewDir,
                                          u_SunColor, u_SunIntensity);
        }

        // -- point lights --
        for (int i = 0; i < u_PointLightCount; i++)
        {
            vec3 toLight =
                u_PointLightPos[i]
                - v_FragPos;

            float dist =
                length(toLight);

            if (dist >
                u_PointLightRange[i])
                continue;

            vec3 lightDir =
                normalize(toLight);

            float strength =
                1.0 -
                (dist /
                 u_PointLightRange[i]);

            result +=
                CalcDiffuseSpecular(
                    lightDir,
                    normal,
                    viewDir,
                    u_PointLightColor[i],
                    u_PointLightIntensity[i]
                )
                * strength;
        }

        // -- spot lights --
        for (int i = 0; i < u_SpotLightCount; i++)
        {
            vec3  toLight  = u_SpotLightPos[i] - v_FragPos;
            float dist     = length(toLight);
            vec3  lightDir = normalize(toLight);

            float theta    = dot(lightDir, normalize(-u_SpotLightDir[i]));
            float epsilon  = u_SpotLightInnerCos[i] - u_SpotLightOuterCos[i];
            float spotFade = clamp((theta - u_SpotLightOuterCos[i]) / epsilon, 0.0, 1.0);

            if (spotFade > 0.0)
            {
                float strength =
                    1.0 - clamp(dist / 25.0, 0.0, 1.0);

                result += CalcDiffuseSpecular(
                    lightDir,
                    normal,
                    viewDir,
                    u_SpotLightColor[i],
                    u_SpotLightIntensity[i])
                * strength
                * spotFade;
            }
        }

        FragColor = vec4(result * u_Color.rgb, u_Color.a);
    }
)";

// ---- flat / unlit (grid + gizmos — unchanged) ----

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
