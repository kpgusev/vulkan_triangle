//
// Created by nix on 9/19/25.
//

#include "../include/Window.hh"

#include "../include/WindowManager.hh"

Window::Window(const int width, const int height, const char *title)
    : window(WindowManager::instance().createWindow(width, height, title)) {}

Window::~Window() { WindowManager::instance().destroyWindow(window); }

GLFWwindow *Window::get() const { return window; }

GLFWwindow *Window::operator*() const { return get(); }

bool Window::shouldClose() const {
  return WindowManager::instance().windowShouldClose(window);
}