if (m_WindowConfiguration.startWindowedFullScreen)
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    m_Window = glfwCreateWindow(mode->width, mode->height, m_WindowConfiguration.Title.c_str(), glfwGetPrimaryMonitor(), NULL);
    int width, height;
    glfwGetWindowSize((GLFWwindow*)m_Window, &width, &height);
    m_WindowConfiguration.Width = width;
    m_WindowConfiguration.Height = height;
}
else
    m_Window = glfwCreateWindow(m_WindowConfiguration.Width, m_WindowConfiguration.Height, m_WindowConfiguration.Title.c_str(), nullptr, NULL);
