#include "Application.h"

#include <GLFW/glfw3.h>

Application* Application::s_Instance = nullptr;

Application::Application(const WindowProps& props)
{
    m_Window = CreateScope<Window>(props);
    m_Window->SetCallbackFunction(BH_BIND_EVENT_FN(OnEvent));

    m_ImGuiLayer = CreateScope<ImGuiLayer>();
    m_Camera = CreateScope<PerspectiveCamera>(45.0f,
        static_cast<float>(m_Window->GetWidth()) / static_cast<float>(m_Window->GetHeight()),
        0.1f, 100.0f);
    m_CameraController = CreateScope<PerspectiveCameraController>(static_cast<PerspectiveCamera*>(m_Camera.get()));

    m_ModelShader = CreateRef<Shader>("../../../assets/shaders/model.glsl");
    m_OutlineShader = CreateRef<Shader>("../../../assets/shaders/outline.glsl");
    m_Model = CreateRef<Model>("../../../assets/models/wheelchair_01_4k/wheelchair_01_4k.obj");
}

Application::~Application()
{
    for (const auto layer : m_LayerStack)
        layer->OnDetach();
    delete s_Instance;
}

void Application::Init(const WindowProps& props)
{
    s_Instance = new Application(props);

    s_Instance->PushOverlay(s_Instance->GetImGuiLayer());
}

void Application::Run()
{
    while (m_IsRunning)
    {
        const float time = m_Timer.Elapsed();
        const Timestep ts = time - m_LastFrameTime;
        m_LastFrameTime = time;

        m_CameraController->OnUpdate(ts);

        /// Temporary for outline testing
        glEnable(GL_DEPTH_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        Renderer::SetClearColor({0.2f, 0.2f, 0.2f, 1.0f});
        Renderer::Clear();

        Renderer::BeginScene(m_Camera);
        Renderer::Submit(m_ModelShader, m_OutlineShader, m_Model);
        Renderer::EndScene();

        for (Layer* layer : m_LayerStack)
            layer->OnUpdate(ts);

        ImGuiLayer::Begin();
        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();
        ImGuiLayer::End();

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(BH_BIND_EVENT_FN(OnKeyPressed));
    dispatcher.Dispatch<WindowCloseEvent>(BH_BIND_EVENT_FN(OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(BH_BIND_EVENT_FN(OnWindowResize));

    m_CameraController->OnEvent(e);

    for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
    {
        (*--it)->OnEvent(e);
        if (e.handled)
            break;
    }
}

void Application::PushLayer(Layer* layer)
{
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer* overlay)
{
    m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    m_IsRunning = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
    Renderer::SetViewport(0, 0, e.GetWidth(), e.GetHeight());
    return true;
}

bool Application::OnKeyPressed(KeyPressedEvent& e)
{
    switch (e.GetKeyCode())
    {
    case GLFW_KEY_ESCAPE:
        m_IsRunning = false;
        break;
    case GLFW_KEY_F11:
        {
            const int monitorWidth = glfwGetVideoMode(glfwGetPrimaryMonitor())->width;
            const int monitorHeight = glfwGetVideoMode(glfwGetPrimaryMonitor())->height;
            if (m_Window->IsFullscreen())
            {
                glfwSetWindowMonitor(m_Window->GetWindowGLFW(),
                    nullptr, 
                    monitorWidth / 4, monitorHeight / 4,
                    monitorWidth / 2, monitorHeight / 2,
                    GLFW_DONT_CARE);
                m_Window->SetFullscreen(false);
            }
            else
            {
                glfwSetWindowMonitor(m_Window->GetWindowGLFW(),
                    glfwGetPrimaryMonitor(), 
                    0, 0,
                    monitorWidth, monitorHeight,
                    GLFW_DONT_CARE);
                m_Window->SetFullscreen(true);
            }
            break;
        }
    }

    return true;
}