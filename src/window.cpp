/*
 * Copyright (C) 2018 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "window.hpp"

Window::Window():
    m_start_time(std::chrono::high_resolution_clock::now()),
    m_window(),
    m_vulkan(),
    m_title("OpenDLV Vulkan renderer"),
    m_height(600),
    m_width(800),
    m_framebuffer_resized(false),
    m_initialized(false),
    m_running(false)
{
  if (!glfwInit()) {
    return;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window = std::shared_ptr<GLFWwindow>(
      glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr),
      [&](GLFWwindow *a_window) {
        glfwDestroyWindow(a_window);
        glfwTerminate();
        std::cout << "DEBUG: Window deleted." << std::endl;
      });

  glfwSetWindowUserPointer(&(*m_window), this);
  glfwSetFramebufferSizeCallback(&(*m_window), OnFramebufferResize);

  glfwSetKeyCallback(&(*m_window), OnKeyPress);
  glfwSetCursorPosCallback(&(*m_window), OnMouseCursorMove);
  glfwSetMouseButtonCallback(&(*m_window), OnMouseButtonPress);

  m_vulkan.reset(new Vulkan(m_window, m_title, m_width, m_height, 
        true));
}

Window::~Window()
{
}

Window& Window::GetInstance()
{
  Window static instance;
  return instance;
}

void Window::OnFramebufferResize(GLFWwindow *a_window, int32_t, int32_t)
{
  auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(a_window));
  window->SetFramebufferResized(true);
}

void Window::OnKeyPress(GLFWwindow *, int32_t a_key, int32_t, int32_t a_action, 
    int32_t)
{
  Window::GetInstance().ParseGlfwButtonInput(a_key, a_action);
}

void Window::OnMouseButtonPress(GLFWwindow *, int32_t a_button, int32_t a_action,
    int32_t)
{
  Window::GetInstance().ParseGlfwButtonInput(a_button, a_action);
}

void Window::OnMouseCursorMove(GLFWwindow *, double a_x, double a_y)
{
  Window::GetInstance().ParseGlfwMouseCursorInput(a_x, a_y);
}

void Window::ParseGlfwButtonInput(int32_t a_button, int32_t)
{
  if (a_button == GLFW_KEY_ESCAPE) {
    m_running = false;
  }
}

void Window::ParseGlfwMouseCursorInput(double, double)
{
}

void Window::SetFramebufferResized(bool a_framebuffer_resized)
{
  m_framebuffer_resized = a_framebuffer_resized;
}

void Window::Start()
{
  m_running = true;

  while (!glfwWindowShouldClose(&(*m_window)) && m_running) {
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = 
      std::chrono::duration<float, std::chrono::seconds::period>(
          current_time - m_start_time).count();
    m_vulkan->DrawFrame(time, m_width, m_height, m_framebuffer_resized);
    m_framebuffer_resized = false;

    glfwPollEvents();
  }
}
