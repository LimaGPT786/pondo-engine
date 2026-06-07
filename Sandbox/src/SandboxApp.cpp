#include <Pondo.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>

// We intentionally do NOT include imgui_impl_glfw.h or call any
// ImGui_ImplGlfw_* functions. Instead we manually populate ImGuiIO
// each frame by polling GLFW directly. This sidesteps every callback
// ordering / context-not-yet-created problem entirely.

// -------------------------------------------------------
//  Shaders
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

// -------------------------------------------------------
//  Ray helpers
// -------------------------------------------------------

struct Ray { glm::vec3 origin, dir; };

static float RayAABB(const Ray& ray, const glm::vec3& mn, const glm::vec3& mx)
{
    float tmin = 0.0f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        if (glm::abs(ray.dir[i]) < 1e-8f) {
            if (ray.origin[i] < mn[i] || ray.origin[i] > mx[i]) return -1.0f;
            continue;
        }
        float invD = 1.0f / ray.dir[i];
        float t0 = (mn[i] - ray.origin[i]) * invD;
        float t1 = (mx[i] - ray.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin) return -1.0f;
    }
    return tmin;
}

static Ray ScreenToRay(const glm::vec2& px, const glm::vec2& vpPos,
    const glm::vec2& vpSize, const glm::mat4& invVP)
{
    float nx = ((px.x - vpPos.x) / vpSize.x) * 2.0f - 1.0f;
    float ny = 1.0f - ((px.y - vpPos.y) / vpSize.y) * 2.0f;
    glm::vec4 n = invVP * glm::vec4(nx, ny, -1.0f, 1.0f); n /= n.w;
    glm::vec4 f = invVP * glm::vec4(nx, ny, 1.0f, 1.0f); f /= f.w;
    return { glm::vec3(n), glm::normalize(glm::vec3(f) - glm::vec3(n)) };
}

static glm::vec2 WorldToScreen(const glm::vec3& world, const glm::mat4& vp,
    const glm::vec2& vpPos, const glm::vec2& vpSize)
{
    glm::vec4 clip = vp * glm::vec4(world, 1.0f);
    if (glm::abs(clip.w) < 1e-6f) return { -9999, -9999 };
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    return {
        vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x,
        vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y
    };
}

// -------------------------------------------------------
//  Grid geometry
// -------------------------------------------------------

struct GridRenderer {
    unsigned int VAO = 0, VBO = 0;
    int lineCount = 0;

    void Build(int halfSize, int step) {
        std::vector<float> v;
        for (int i = -halfSize; i <= halfSize; i += step) {
            float f = (float)i;
            v.insert(v.end(), { f,0,-(float)halfSize, f,0,(float)halfSize });
            v.insert(v.end(), { -(float)halfSize,0,f, (float)halfSize,0,f });
        }
        lineCount = (int)v.size() / 3;
        glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }
    void Draw() const { glBindVertexArray(VAO); glDrawArrays(GL_LINES, 0, lineCount); glBindVertexArray(0); }
};

// -------------------------------------------------------
//  Arrow gizmo geometry
// -------------------------------------------------------

struct ArrowGizmo {
    unsigned int VAO = 0, VBO = 0;
    int vertCount = 0;

    void Build() {
        float s = 0.07f, tip = 1.0f, base = 0.78f;
        std::vector<float> v = {
            0,0,0, 0,tip,0,
            0,tip,0, s,base,0,  0,tip,0,-s,base,0,
            0,tip,0, 0,base,s,  0,tip,0, 0,base,-s,
        };
        vertCount = (int)v.size() / 3;
        glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }
    void Draw() const { glBindVertexArray(VAO); glDrawArrays(GL_LINES, 0, vertCount); glBindVertexArray(0); }
};

// -------------------------------------------------------
//  SceneLayer
// -------------------------------------------------------

class SceneLayer : public Pondo::Layer {
public:
    SceneLayer() : Layer("SceneLayer") {}

    void SetFramebuffer(std::shared_ptr<Pondo::Framebuffer> fb) { m_Framebuffer = fb; }
    void SetViewportFocused(bool v) { m_ViewportFocused = v; }
    void SetViewportRect(float x, float y, float w, float h) { m_VpPos = { x,y }; m_VpSize = { w,h }; }

    Pondo::Scene* GetScene() { return m_Scene.get(); }
    Pondo::Entity* GetSelectedEntity() { return m_SelectedEntity; }
    void           SetSelectedEntity(Pondo::Entity* e) { m_SelectedEntity = e; }
    Pondo::Camera* GetCamera() { return m_Camera.get(); }

    void TryPickEntity(glm::vec2 px) {
        printf("[PICK] TryPickEntity px=(%.0f,%.0f) vpPos=(%.0f,%.0f) vpSize=(%.0f,%.0f)\n",
            px.x, px.y, m_VpPos.x, m_VpPos.y, m_VpSize.x, m_VpSize.y);
        if (m_VpSize.x <= 0 || m_VpSize.y <= 0) { printf("[PICK] bad vpSize, abort\n"); return; }
        Ray ray = ScreenToRay(px, m_VpPos, m_VpSize,
            glm::inverse(m_Camera->GetViewProjection()));
        printf("[PICK] ray origin=(%.2f,%.2f,%.2f) dir=(%.2f,%.2f,%.2f)\n",
            ray.origin.x, ray.origin.y, ray.origin.z, ray.dir.x, ray.dir.y, ray.dir.z);
        Pondo::Entity* best = nullptr; float bestT = 1e30f;
        for (auto& ep : m_Scene->GetEntities()) {
            if (!ep->GetMesh()) continue;
            glm::vec3 c = ep->GetTransform().Position;
            glm::vec3 h = glm::abs(ep->GetTransform().Scale) * 0.5f;
            float t = RayAABB(ray, c - h, c + h);
            printf("[PICK]   entity='%s' center=(%.1f,%.1f,%.1f) half=(%.1f,%.1f,%.1f) t=%.3f\n",
                ep->GetTag().c_str(), c.x, c.y, c.z, h.x, h.y, h.z, t);
            if (t > 0.0f && t < bestT) { bestT = t; best = ep.get(); }
        }
        m_SelectedEntity = best;
        printf("[PICK] selected: %s\n", best ? best->GetTag().c_str() : "NONE");
        fflush(stdout);
    }

    int GizmoAxisHit(glm::vec2 px, float r = 18.0f) {
        if (!m_SelectedEntity) return -1;
        glm::vec3 o = m_SelectedEntity->GetTransform().Position;
        float scale = glm::length(m_Camera->GetPosition() - o) * 0.15f;
        glm::vec3 tips[3] = { o + glm::vec3(scale,0,0), o + glm::vec3(0,scale,0), o + glm::vec3(0,0,scale) };
        for (int i = 0; i < 3; ++i)
            if (glm::length(px - WorldToScreen(tips[i], m_Camera->GetViewProjection(), m_VpPos, m_VpSize)) < r)
                return i;
        return -1;
    }

    void BeginGizmoDrag(int axis, glm::vec2 px) {
        m_DragAxis = axis; m_DragStartPx = px;
        m_DragStartPos = m_SelectedEntity->GetTransform().Position;
    }
    void UpdateGizmoDrag(glm::vec2 px) {
        if (m_DragAxis < 0 || !m_SelectedEntity) return;
        static const glm::vec3 kAxes[3] = { {1,0,0},{0,1,0},{0,0,1} };
        glm::vec3 wa = kAxes[m_DragAxis];
        glm::vec2 p0 = WorldToScreen(m_DragStartPos, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
        glm::vec2 p1 = WorldToScreen(m_DragStartPos + wa, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
        glm::vec2 sd = p1 - p0;
        float sl = glm::length(sd);
        if (sl < 1.0f) return;
        sd /= sl;
        float delta = glm::dot(px - m_DragStartPx, sd) / sl;
        m_SelectedEntity->GetTransform().Position = m_DragStartPos + wa * delta;
    }
    void EndGizmoDrag() { m_DragAxis = -1; }
    bool IsDraggingGizmo() const { return m_DragAxis >= 0; }

    Pondo::Entity* SpawnEntity(const std::string& name,
        std::shared_ptr<Pondo::Mesh> mesh, glm::vec4 color,
        glm::vec3 pos = { 0,0.5f,0 })
    {
        auto* e = m_Scene->CreateEntity(name);
        e->GetTransform().Position = pos;
        e->SetMesh(std::move(mesh));
        e->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
        e->GetMaterial()->Mat->SetColor(color);
        return e;
    }

    void DeleteSelected() {
        if (!m_SelectedEntity) return;
        uint32_t id = m_SelectedEntity->GetID();
        m_SelectedEntity = nullptr;
        m_Scene->DestroyEntity(id);
    }

    void OnAttach() override {
        m_Camera = std::make_shared<Pondo::Camera>(60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        m_Shader = std::make_shared<Pondo::Shader>(s_VertSrc, s_FragSrc);
        m_FlatShader = std::make_shared<Pondo::Shader>(s_FlatVertSrc, s_FlatFragSrc);
        m_Grid.Build(50, 1);
        m_Arrow.Build();
        m_Scene = std::make_unique<Pondo::Scene>("Main Scene");

        SpawnEntity("Cube", Pondo::Mesh::CreateCube(), { 0.8f,0.35f,0.2f,1 }, { 0,0.5f,0 });
        SpawnEntity("Sphere", Pondo::Mesh::CreateSphere(), { 0.2f,0.55f,0.85f,1 }, { 2.5f,0.5f,0 });

        auto* ground = m_Scene->CreateEntity("Ground");
        ground->GetTransform().Scale = { 20,1,20 };
        ground->GetTransform().Position = { 0,-0.5f,0 };
        ground->SetMesh(Pondo::Mesh::CreatePlane(1.0f));
        ground->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
        ground->GetMaterial()->Mat->SetColor({ 0.3f,0.5f,0.3f,1 });
    }

    void OnUpdate(Pondo::Timestep ts) override {
        if (!m_ViewportFocused) return;
        float spd = 5.0f * ts.GetSeconds();
        float sens = 50.0f * ts.GetSeconds();
        glm::vec3 pos = m_Camera->GetPosition();
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_W)) pos += m_Camera->GetForward() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_S)) pos -= m_Camera->GetForward() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_A)) pos -= m_Camera->GetRight() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_D)) pos += m_Camera->GetRight() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_E)) pos.y += spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_Q)) pos.y -= spd;
        m_Camera->SetPosition(pos);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT))  m_Camera->SetYaw(m_Camera->GetYaw() - sens);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT)) m_Camera->SetYaw(m_Camera->GetYaw() + sens);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_UP))    m_Camera->SetPitch(m_Camera->GetPitch() + sens);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_DOWN))  m_Camera->SetPitch(m_Camera->GetPitch() - sens);

        if (Pondo::Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT) && !IsDraggingGizmo()) {
            auto [mx, my] = Pondo::Input::GetMousePosition();
            if (m_FirstMouseLook) { m_LastMouseX = mx; m_LastMouseY = my; m_FirstMouseLook = false; }
            m_Camera->ProcessMouseLook(mx - m_LastMouseX, my - m_LastMouseY);
            m_LastMouseX = mx; m_LastMouseY = my;
        }
        else { m_FirstMouseLook = true; }
    }

    void OnRender() override {
        if (!m_Framebuffer) return;
        m_Framebuffer->Bind();
        Pondo::Renderer::SetClearColor(0.12f, 0.12f, 0.13f, 1.0f);
        Pondo::Renderer::Clear();
        Pondo::Renderer::BeginScene(*m_Camera);

        glEnable(GL_DEPTH_TEST);
        for (auto& ep : m_Scene->GetEntities()) {
            auto* mc = ep->GetMesh(); auto* mat = ep->GetMaterial();
            if (!mc || !mat || !mc->MeshData || !mat->Mat) continue;
            Pondo::Renderer::SubmitMesh(mc->MeshData, mat->Mat, ep->GetTransform().GetTransform());
        }

        m_FlatShader->Bind();
        m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());
        m_FlatShader->SetMat4("u_Transform", glm::mat4(1.0f));
        m_FlatShader->SetFloat4("u_Color", { 0.35f,0.35f,0.38f,1 });
        glLineWidth(1.0f); m_Grid.Draw();

        if (m_SelectedEntity) {
            auto& tf = m_SelectedEntity->GetTransform();
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            m_FlatShader->Bind();
            m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());
            m_FlatShader->SetMat4("u_Transform",
                glm::translate(glm::mat4(1), tf.Position) *
                glm::scale(glm::mat4(1), tf.Scale * 1.05f));
            m_FlatShader->SetFloat4("u_Color", { 1,0.85f,0.1f,1 });
            auto* mc = m_SelectedEntity->GetMesh();
            if (mc && mc->MeshData) {
                mc->MeshData->Bind();
                glDrawElements(GL_TRIANGLES, mc->MeshData->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                mc->MeshData->Unbind();
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            DrawGizmo();
            glEnable(GL_DEPTH_TEST);
        }

        Pondo::Renderer::EndScene();
        m_Framebuffer->Unbind();
    }

    void OnEvent(Pondo::Event& e) override {
        Pondo::EventDispatcher d(e);
        d.Dispatch<Pondo::WindowResizeEvent>([this](Pondo::WindowResizeEvent& ev) {
            if (ev.GetWidth() > 0 && ev.GetHeight() > 0)
                m_Camera->SetAspectRatio((float)ev.GetWidth() / ev.GetHeight());
            return false;
            });
    }

private:
    void DrawGizmo() {
        glm::vec3 o = m_SelectedEntity->GetTransform().Position;
        float scale = glm::length(m_Camera->GetPosition() - o) * 0.15f;
        glLineWidth(3.0f); glDisable(GL_DEPTH_TEST);
        struct Ax { glm::vec3 axis; glm::vec4 col, hov; };
        Ax axes[3] = {
            {{1,0,0},{0.95f,0.25f,0.25f,1},{1,0.6f,0.6f,1}},
            {{0,1,0},{0.25f,0.90f,0.25f,1},{0.6f,1,0.6f,1}},
            {{0,0,1},{0.25f,0.45f,0.95f,1},{0.6f,0.7f,1,1}},
        };
        for (int i = 0; i < 3; ++i)
            DrawArrow(o, axes[i].axis, scale, m_DragAxis == i ? axes[i].hov : axes[i].col);
        glLineWidth(1.0f); glEnable(GL_DEPTH_TEST);
    }

    void DrawArrow(const glm::vec3& origin, const glm::vec3& axis,
        float scale, const glm::vec4& color)
    {
        glm::vec3 up = { 0,1,0 };
        glm::mat4 rot = glm::mat4(1);
        float d = glm::dot(up, axis);
        if (d < -0.9999f)      rot = glm::rotate(glm::mat4(1), glm::pi<float>(), glm::vec3(1, 0, 0));
        else if (d < 0.9999f)  rot = glm::rotate(glm::mat4(1), glm::acos(glm::clamp(d, -1.f, 1.f)),
            glm::normalize(glm::cross(up, axis)));
        glm::mat4 t = glm::translate(glm::mat4(1), origin) * rot * glm::scale(glm::mat4(1), glm::vec3(scale));
        m_FlatShader->SetMat4("u_Transform", t);
        m_FlatShader->SetFloat4("u_Color", color);
        m_Arrow.Draw();
    }

    std::shared_ptr<Pondo::Camera>      m_Camera;
    std::shared_ptr<Pondo::Shader>      m_Shader, m_FlatShader;
    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;
    std::unique_ptr<Pondo::Scene>       m_Scene;
    Pondo::Entity* m_SelectedEntity = nullptr;
    GridRenderer m_Grid; ArrowGizmo m_Arrow;
    bool  m_ViewportFocused = false, m_FirstMouseLook = true;
    float m_LastMouseX = 0, m_LastMouseY = 0;
    glm::vec2 m_VpPos = { 0,0 }, m_VpSize = { 1280,720 };
    int m_DragAxis = -1;
    glm::vec2 m_DragStartPx = {};
    glm::vec3 m_DragStartPos = {};
};

// -------------------------------------------------------
//  EditorLayer  —  manually feeds ImGuiIO each frame
// -------------------------------------------------------

class EditorLayer : public Pondo::Layer {
public:
    EditorLayer(SceneLayer* sl) : Layer("EditorLayer"), m_SceneLayer(sl) {}

    void OnAttach() override {
        m_Window = static_cast<GLFWwindow*>(
            Pondo::Application::Get().GetWindow().GetNativeWindow());

        int w = Pondo::Application::Get().GetWindow().GetWidth();
        int h = Pondo::Application::Get().GetWindow().GetHeight();

        Pondo::FramebufferSpec spec;
        spec.Width = (unsigned)w; spec.Height = (unsigned)h;
        m_Framebuffer = std::make_shared<Pondo::Framebuffer>(spec);
        m_SceneLayer->SetFramebuffer(m_Framebuffer);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetMainViewport()->PlatformHandleRaw = glfwGetWin32Window(m_Window);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.DisplaySize = { (float)w, (float)h };
        io.DisplayFramebufferScale = { 1,1 };
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendPlatformName = "pondo_glfw_manual";
        // KeyMap was removed in ImGui 1.87 — keys are handled via AddKeyEvent now.

        ImGui::StyleColorsDark();
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 4.0f; s.FrameRounding = 3.0f;
        s.Colors[ImGuiCol_WindowBg] = { 0.13f,0.13f,0.14f,1 };
        s.Colors[ImGuiCol_TitleBg] = { 0.10f,0.10f,0.11f,1 };
        s.Colors[ImGuiCol_TitleBgActive] = { 0.16f,0.29f,0.48f,1 };
        s.Colors[ImGuiCol_Header] = { 0.20f,0.35f,0.55f,0.6f };
        s.Colors[ImGuiCol_HeaderHovered] = { 0.26f,0.47f,0.70f,0.8f };
        s.Colors[ImGuiCol_FrameBg] = { 0.18f,0.18f,0.20f,1 };
        s.Colors[ImGuiCol_Button] = { 0.20f,0.35f,0.55f,1 };
        s.Colors[ImGuiCol_ButtonHovered] = { 0.26f,0.47f,0.70f,1 };

        // Only init the OpenGL renderer backend — no GLFW backend at all
        ImGui_ImplOpenGL3_Init("#version 450");

        printf("[INIT] m_Window = %p\n", (void*)m_Window);
        fflush(stdout);
    }

    void OnDetach() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

    void OnEvent(Pondo::Event& e) override {
        Pondo::EventDispatcher d(e);
        d.Dispatch<Pondo::WindowResizeEvent>([](Pondo::WindowResizeEvent& ev) {
            ImGui::GetIO().DisplaySize = { (float)ev.GetWidth(), (float)ev.GetHeight() };
            return false;
            });
    }

    void OnRender() override {
        int w = Pondo::Application::Get().GetWindow().GetWidth();
        int h = Pondo::Application::Get().GetWindow().GetHeight();

        // ── Manually populate ImGuiIO from GLFW ──────────────────────
        // This is what imgui_impl_glfw normally does, but by doing it
        // ourselves we avoid every callback / context-ordering problem.
        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = { (float)w, (float)h };
        io.DisplayFramebufferScale = { 1, 1 };

        // Delta time
        float now = (float)glfwGetTime();
        io.DeltaTime = (m_LastTime > 0.0f) ? (now - m_LastTime) : (1.0f / 60.0f);
        if (io.DeltaTime <= 0.0f) io.DeltaTime = 1.0f / 60.0f;
        m_LastTime = now;

        // Use the engine's own Input system — it calls glfwGetCursorPos
        // on the application window which is guaranteed to be correct.
        {
            auto [mx, my] = Pondo::Input::GetMousePosition();
            io.MousePos = { mx, my };
        }

        // Mouse buttons — poll current state AND detect click edges ourselves
        // because ImGui::IsMouseClicked requires seeing a false->true transition.
        for (int i = 0; i < 5; ++i) {
            // A press is registered if GLFW reports it OR if we haven't yet
            // cleared a queued press from a callback (not used here, but safe).
            io.MouseDown[i] = glfwGetMouseButton(m_Window, i) == GLFW_PRESS;
        }

        // Keyboard modifiers
        io.KeyCtrl = glfwGetKey(m_Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
            || glfwGetKey(m_Window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        io.KeyShift = glfwGetKey(m_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(m_Window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        io.KeyAlt = glfwGetKey(m_Window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
            || glfwGetKey(m_Window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
        io.KeySuper = glfwGetKey(m_Window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS
            || glfwGetKey(m_Window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;

        // ── Begin frame ───────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        // ── Menu bar ─────────────────────────────────────────────────
        ImGui::SetNextWindowPos({ 0,0 });
        ImGui::SetNextWindowSize({ (float)w,0 });
        ImGui::SetNextWindowBgAlpha(0.95f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("##Menu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar(2);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) Pondo::Application::Get().Close();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Scene", nullptr, &m_ShowScene);
                ImGui::MenuItem("Properties", nullptr, &m_ShowProps);
                ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Cube"))   CreateEntity(0);
                if (ImGui::MenuItem("Sphere")) CreateEntity(1);
                if (ImGui::MenuItem("Plane"))  CreateEntity(2);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        float menuH = ImGui::GetWindowHeight();
        ImGui::End();

        const float sideW = 260.0f;
        const float topY = menuH;
        const float contentH = (float)h - topY;
        float leftW = m_ShowScene ? sideW : 0.0f;
        float rightW = (m_ShowProps || m_ShowStats) ? sideW : 0.0f;

        // ── Scene panel ──────────────────────────────────────────────
        if (m_ShowScene) {
            ImGui::SetNextWindowPos({ 0,topY });
            ImGui::SetNextWindowSize({ sideW,contentH });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("Scene##panel", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::PopStyleVar();

            ImGui::Text("Scene: %s", m_SceneLayer->GetScene()->GetName().c_str());
            ImGui::Separator();
            for (auto& ep : m_SceneLayer->GetScene()->GetEntities()) {
                bool sel = (m_SceneLayer->GetSelectedEntity() == ep.get());
                ImGui::PushID((int)ep->GetID());
                if (ImGui::Selectable(ep->GetTag().c_str(), sel))
                    m_SceneLayer->SetSelectedEntity(ep.get());
                ImGui::PopID();
            }
            ImGui::Separator();
            if (ImGui::Button("+ Cube"))   CreateEntity(0);
            ImGui::SameLine();
            if (ImGui::Button("+ Sphere")) CreateEntity(1);
            ImGui::SameLine();
            if (ImGui::Button("+ Plane"))  CreateEntity(2);
            if (m_SceneLayer->GetSelectedEntity()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.65f,0.15f,0.15f,1 });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.80f,0.20f,0.20f,1 });
                if (ImGui::Button("Delete Selected")) m_SceneLayer->DeleteSelected();
                ImGui::PopStyleColor(2);
            }
            ImGui::End();
        }

        // ── Properties panel ─────────────────────────────────────────
        if (m_ShowProps || m_ShowStats) {
            ImGui::SetNextWindowPos({ (float)w - sideW, topY });
            ImGui::SetNextWindowSize({ sideW, contentH });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("Properties##panel", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::PopStyleVar();

            if (m_ShowStats) {
                ImGui::Text("Pondo Engine");
                ImGui::Text("%.1f FPS  (%.3f ms)", io.Framerate, 1000.0f / io.Framerate);
                ImGui::Text("Entities: %zu", m_SceneLayer->GetScene()->GetEntities().size());
                if (auto* cam = m_SceneLayer->GetCamera()) {
                    glm::vec3 p = cam->GetPosition();
                    ImGui::Text("Cam: (%.1f, %.1f, %.1f)", p.x, p.y, p.z);
                    ImGui::Text("Yaw: %.1f  Pitch: %.1f", cam->GetYaw(), cam->GetPitch());
                }
                ImGui::Separator();
            }

            if (m_ShowProps) {
                if (Pondo::Entity* sel = m_SceneLayer->GetSelectedEntity()) {
                    char buf[128]; strncpy_s(buf, sel->GetTag().c_str(), sizeof(buf));
                    if (ImGui::InputText("Name", buf, sizeof(buf))) sel->SetTag(buf);
                    ImGui::Separator();
                    ImGui::Text("Transform");
                    auto& tf = sel->GetTransform();
                    float pos[3] = { tf.Position.x, tf.Position.y, tf.Position.z };
                    if (ImGui::DragFloat3("Position", pos, 0.05f)) tf.Position = { pos[0],pos[1],pos[2] };
                    float rot[3] = { tf.Rotation.x, tf.Rotation.y, tf.Rotation.z };
                    if (ImGui::DragFloat3("Rotation", rot, 1.0f))  tf.Rotation = { rot[0],rot[1],rot[2] };
                    float scl[3] = { tf.Scale.x, tf.Scale.y, tf.Scale.z };
                    if (ImGui::DragFloat3("Scale", scl, 0.05f, 0.001f, 1000.0f)) tf.Scale = { scl[0],scl[1],scl[2] };
                    if (auto* mc = sel->GetMaterial(); mc && mc->Mat) {
                        ImGui::Separator(); ImGui::Text("Material");
                        glm::vec4 col = mc->Mat->GetColor();
                        float ca[4] = { col.r,col.g,col.b,col.a };
                        if (ImGui::ColorEdit4("Color", ca)) mc->Mat->SetColor({ ca[0],ca[1],ca[2],ca[3] });
                    }
                }
                else {
                    ImGui::TextDisabled("Nothing selected.");
                    ImGui::TextDisabled("Click an object in the viewport");
                    ImGui::TextDisabled("or the scene panel.");
                }
            }
            ImGui::End();
        }

        // ── Viewport ─────────────────────────────────────────────────
        float vpX = leftW;
        float vpW = (float)w - leftW - rightW;
        ImGui::SetNextWindowPos({ vpX, topY });
        ImGui::SetNextWindowSize({ vpW, contentH });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Viewport##panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar(2);

        m_SceneLayer->SetViewportFocused(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());

        ImVec2 contentPos = ImGui::GetCursorScreenPos();
        ImVec2 panelSize = ImGui::GetContentRegionAvail();

        if (panelSize.x > 0 && panelSize.y > 0) {
            unsigned pw = (unsigned)panelSize.x, ph = (unsigned)panelSize.y;
            if (pw != m_Framebuffer->GetSpec().Width || ph != m_Framebuffer->GetSpec().Height) {
                m_Framebuffer->Resize(pw, ph);
                Pondo::WindowResizeEvent ev(pw, ph);
                m_SceneLayer->OnEvent(ev);
            }
            m_SceneLayer->SetViewportRect(contentPos.x, contentPos.y, panelSize.x, panelSize.y);

            ImGui::Image((ImTextureID)(uintptr_t)m_Framebuffer->GetColorAttachmentID(),
                panelSize, { 0,1 }, { 1,0 });

            glm::vec2 mouse = { io.MousePos.x, io.MousePos.y };

            // Do NOT use IsItemHovered() — NoBringToFrontOnFocus keeps the
            // viewport behind side panels in ImGui's Z-order, so IsItemHovered
            // always returns false even when the mouse is visually over it.
            // Test the viewport rect directly instead.
            bool lmbDown = Pondo::Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            static int dbgFrame = 0;
            if (++dbgFrame % 60 == 0) {
                printf("[FRAME] lmbDown=%d mouse=(%.0f,%.0f) vp=(%.0f,%.0f)+(%.0f,%.0f)\n",
                    (int)lmbDown, mouse.x, mouse.y,
                    contentPos.x, contentPos.y, panelSize.x, panelSize.y);
                fflush(stdout);
            }
            bool overViewport = mouse.x >= contentPos.x
                && mouse.x < contentPos.x + panelSize.x
                && mouse.y >= contentPos.y
                && mouse.y < contentPos.y + panelSize.y;

            // Detect left-click leading edge ourselves (was up last frame, down now)
            bool lmbClicked = lmbDown && !m_PrevLmbDown;
            m_PrevLmbDown = lmbDown;

            // Gizmo drag update
            if (m_SceneLayer->IsDraggingGizmo()) {
                if (lmbDown) m_SceneLayer->UpdateGizmoDrag(mouse);
                else         m_SceneLayer->EndGizmoDrag();
            }

            // DEBUG — print every frame so we can see what's happening
            if (lmbDown || lmbClicked || overViewport) {
                printf("[CLICK] mouse=(%.0f,%.0f) vp=(%.0f,%.0f,%.0f,%.0f) over=%d lmbDown=%d lmbClicked=%d\n",
                    mouse.x, mouse.y,
                    contentPos.x, contentPos.y, contentPos.x + panelSize.x, contentPos.y + panelSize.y,
                    (int)overViewport, (int)lmbDown, (int)lmbClicked);
                fflush(stdout);
            }

            // Left click in viewport: gizmo first, then entity pick
            if (overViewport && lmbClicked && !m_SceneLayer->IsDraggingGizmo()) {
                printf("[PICK] Trying to pick at mouse=(%.0f,%.0f)\n", mouse.x, mouse.y);
                fflush(stdout);
                int axis = m_SceneLayer->GizmoAxisHit(mouse);
                if (axis >= 0) m_SceneLayer->BeginGizmoDrag(axis, mouse);
                else           m_SceneLayer->TryPickEntity(mouse);
            }
        }
        ImGui::End();

        // ── Flush ─────────────────────────────────────────────────────
        glDisable(GL_DEPTH_TEST);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glEnable(GL_DEPTH_TEST);
    }

private:
    void CreateEntity(int meshType) {
        static int count = 0;
        const char* names[] = { "Cube","Sphere","Plane" };
        glm::vec4 colors[] = { {0.8f,0.35f,0.2f,1},{0.2f,0.55f,0.85f,1},{0.55f,0.55f,0.55f,1} };
        std::shared_ptr<Pondo::Mesh> mesh;
        switch (meshType) {
        case 1:  mesh = Pondo::Mesh::CreateSphere();    break;
        case 2:  mesh = Pondo::Mesh::CreatePlane(1.0f); break;
        default: mesh = Pondo::Mesh::CreateCube();
        }
        auto* e = m_SceneLayer->SpawnEntity(
            std::string(names[meshType]) + std::to_string(++count),
            std::move(mesh), colors[meshType], { 0,0.5f,0 });
        m_SceneLayer->SetSelectedEntity(e);
    }

    GLFWwindow* m_Window = nullptr;
    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;
    SceneLayer* m_SceneLayer = nullptr;
    float                               m_LastTime = 0.0f;
    bool m_PrevLmbDown = false;
    bool m_ShowScene = true, m_ShowProps = true, m_ShowStats = true;
};

// -------------------------------------------------------
//  Application
// -------------------------------------------------------

class Sandbox : public Pondo::Application {
public:
    Sandbox() {
        gladLoadGLLoader((GLADloadproc)Pondo::Application::GetProcAddress);
        m_SceneLayer = new SceneLayer();
        m_EditorLayer = new EditorLayer(m_SceneLayer);
        PushLayer(m_SceneLayer);
        PushOverlay(m_EditorLayer);
    }
    ~Sandbox() { delete m_SceneLayer; delete m_EditorLayer; }
private:
    SceneLayer* m_SceneLayer = nullptr;
    EditorLayer* m_EditorLayer = nullptr;
};

Pondo::Application* Pondo::CreateApplication() { return new Sandbox(); }
