#include <iostream>
#include "Window.h"
#include "Events.h"
#include "../graphics/ImageData.h"

#ifdef USE_DIRECTX
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define NOMINMAX
#include <GLFW/glfw3native.h>
#include "../directx/window/DXDevice.hpp"
#include "../directx/util/TextureUtil.hpp"
#elif USE_OPENGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../graphics/Mesh.h"
#include "../graphics/Texture.h"
#include "../graphics/Shader.h"
#endif // USE_DIRECTX

using glm::vec4;
using std::cout;
using std::cerr;
using std::endl;

GLFWwindow* Window::window = nullptr;
DisplaySettings* Window::settings = nullptr;
std::stack<vec4> Window::scissorStack;
vec4 Window::scissorArea;
uint Window::width = 0;
uint Window::height = 0;
int Window::posX = 0;
int Window::posY = 0;

void cursor_position_callback(GLFWwindow*, double xpos, double ypos) {
    Events::setPosition(xpos, ypos);
}

void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    Events::setButton(button, action == GLFW_PRESS);
}

void key_callback(GLFWwindow*, int key, int scancode, int action, int /*mode*/) {
	if (key == GLFW_KEY_UNKNOWN) return;
	if (action == GLFW_PRESS) {
        Events::setKey(key, true);
		Events::pressedKeys.push_back(key);
	}
	else if (action == GLFW_RELEASE) {
        Events::setKey(key, false);
	}
	else if (action == GLFW_REPEAT) {
		Events::pressedKeys.push_back(key);
	}
}

void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {
	Events::scroll += yoffset;
}

bool Window::isMaximized() {
	return glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}

bool Window::isIconified() {
	return glfwGetWindowAttrib(window, GLFW_ICONIFIED);
}
bool Window::isFocused()
{
	return glfwGetWindowAttrib(window, GLFW_FOCUSED);
}

void window_size_callback(GLFWwindow*, int width, int height) {
	if (Window::isFocused() && width && height) {
#ifdef USE_DIRECTX
		DXDevice::onWindowResize(width, height);
#elif USE_OPENGL
		glViewport(0, 0, width, height);
#endif // !USE_DIRECTX
		Window::width = width;
		Window::height = height;
	}

	if (!Window::isFullscreen() && !Window::isMaximized()) {
		Window::getSettings()->width = width;
		Window::getSettings()->height = height;
	}
	Window::resetScissor();
}

void character_callback(GLFWwindow* window, unsigned int codepoint){
	Events::codepoints.push_back(codepoint);
}

const char* glfwErrorName(int error) {
	switch (error) {
		case GLFW_NO_ERROR: return "no error";
		case GLFW_NOT_INITIALIZED: return "not initialized";
		case GLFW_NO_CURRENT_CONTEXT: return "no current context";
		case GLFW_INVALID_ENUM: return "invalid enum";
		case GLFW_INVALID_VALUE: return "invalid value";
		case GLFW_OUT_OF_MEMORY: return "out of memory";
		case GLFW_API_UNAVAILABLE: return "api unavailable";
		case GLFW_VERSION_UNAVAILABLE: return "version unavailable";
		case GLFW_PLATFORM_ERROR: return "platform error";
		case GLFW_FORMAT_UNAVAILABLE: return "format unavailable";
		case GLFW_NO_WINDOW_CONTEXT: return "no window context";
		default: return "unknown error";
	}
}

void error_callback(int error, const char* description) {
	cerr << "GLFW error [0x" << std::hex << error << "]: ";
	cerr << glfwErrorName(error) << endl;
	if (description) {
		cerr << description << endl;
	}
}

int Window::initialize(DisplaySettings& settings){
	Window::settings = &settings;
	Window::width = settings.width;
	Window::height = settings.height;

	glfwSetErrorCallback(error_callback);
	if (glfwInit() == GLFW_FALSE) {
		cerr << "Failed to initialize GLFW" << endl;
		return -1;
	}

#ifdef USE_DIRECTX
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif USE_OPENGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#else
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, settings.samples);
#endif // USE_DIRECTX

	window = glfwCreateWindow(width, height, settings.title.c_str(), nullptr, nullptr);
	if (window == nullptr){
		cerr << "Failed to create GLFW Window" << endl;
		glfwTerminate();
		return -1;
	}

#ifdef USE_DIRECTX
	HWND windowHandle = glfwGetWin32Window(window);
	DXDevice::initialize(windowHandle, width, height);
	DXDevice::setSwapInterval(settings.swapInterval);

	auto adapterDesc = DXDevice::getAdapterDesc();

	std::wcout << L"Renderer: " << adapterDesc.Description << std::endl;
#elif USE_OPENGL
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK){
		cerr << "Failed to initialize GLEW: " << std::endl;
		cerr << glewGetErrorString(glewErr) << std::endl;
		return -1;
	}

	glViewport(0,0, width, height);
	glClearColor(0.0f,0.0f,0.0f, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glfwSwapInterval(settings.swapInterval);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	cout << "GL Vendor: " << (char*)vendor << endl;
	cout << "GL Renderer: " << (char*)renderer << endl;
#endif // USE_DIRECTX

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetCharCallback(window, character_callback);
	glfwSetScrollCallback(window, scroll_callback);
	if (settings.fullscreen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
	}

	return 0;
}

void Window::clear() {
#ifdef USE_DIRECTX
	DXDevice::clear();
#elif USE_OPENGL
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif // USE_DIRECTX
}

void Window::clearDepth() {
#ifdef USE_DIRECTX
	DXDevice::clearDepth();
#elif USE_OPENGL
	glClear(GL_DEPTH_BUFFER_BIT);
#endif // USE_DIRECTX
}

void Window::setBgColor(glm::vec3 color) {
	glClearColor(color.r, color.g, color.b, 1.0f);
}

void Window::viewport(int x, int y, int width, int height){
#ifdef USE_DIRECTX
	DXDevice::resizeViewPort(x, y, width, height);
#elif USE_OPENGL
	glViewport(x, y, width, height);
#endif // USE_DIRECTX
}

void Window::setCursorMode(int mode){
	glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void Window::resetScissor() {
	scissorArea = vec4(0.0f, 0.0f, width, height);
	scissorStack = std::stack<vec4>();
#ifdef USE_DIRECTX
	DXDevice::setScissorTest(false);
#elif USE_OPENGL
	glDisable(GL_SCISSOR_TEST);
#endif // USE_DIRECTX
}

void Window::pushScissor(vec4 area) {
	if (scissorStack.empty()) {
#ifdef USE_DIRECTX
		DXDevice::setScissorTest(true);
#elif USE_OPENGL
		glEnable(GL_SCISSOR_TEST);
#endif // USE_DIRECTX
	}
	scissorStack.push(scissorArea);

	area.z += area.x;
	area.w += area.y;

	area.x = fmax(area.x, scissorArea.x);
	area.y = fmax(area.y, scissorArea.y);

	area.z = fmin(area.z, scissorArea.z);
	area.w = fmin(area.w, scissorArea.w);

	if (area.z < 0.0f || area.w < 0.0f) {
#ifdef USE_DIRECTX
		DXDevice::setScissorRect(0, 0, 0, 0);
#elif USE_OPENGL
		glScissor(0, 0, 0, 0);
#endif // USE_DIRECTX
	} else {
#ifdef USE_DIRECTX
		DXDevice::setScissorRect(area.x, area.y, 
				  std::max(0, int(area.z-area.x)), 
				  std::max(0, int(area.w-area.y)));
#elif USE_OPENGL
		glScissor(area.x, Window::height-area.w, 
				  std::max(0, int(area.z-area.x)), 
				  std::max(0, int(area.w-area.y)));
#endif // USE_DIRECTX
	}
	scissorArea = area;
}

void Window::popScissor() {
	if (scissorStack.empty()) {
		std::cerr << "warning: extra Window::popScissor call" << std::endl;
		return;
	}
	vec4 area = scissorStack.top();
	scissorStack.pop();
	if (area.z < 0.0f || area.w < 0.0f) {
#ifdef USE_DIRECTX
		DXDevice::setScissorRect(0, 0, 0, 0);
#elif USE_OPENGL
		glScissor(0, 0, 0, 0);
#endif // USE_DIRECTX
	} else {
#ifdef USE_DIRECTX
		DXDevice::setScissorRect(area.x, area.y, 
								std::max(0, int(area.z-area.x)), 
								std::max(0, int(area.w-area.y)));
#elif USE_OPENGL
		glScissor(area.x, Window::height-area.w, 
				  std::max(0, int(area.z-area.x)), 
				  std::max(0, int(area.w-area.y)));
#endif // USE_DIRECTX
	}
	if (scissorStack.empty()) {
#ifdef USE_DIRECTX
		DXDevice::setScissorTest(false);
#elif USE_OPENGL
		glDisable(GL_SCISSOR_TEST);
#endif // USE_DIRECTX
	}
	scissorArea = area;
}

void Window::terminate(){
	glfwTerminate();
#ifdef USE_DIRECTX
	DXDevice::terminate();
#endif // USE_DIRECTX

}

bool Window::isShouldClose(){
	return glfwWindowShouldClose(window);
}

void Window::setShouldClose(bool flag){
	glfwSetWindowShouldClose(window, flag);
}

void Window::swapInterval(int interval){
#ifdef USE_DIRECTX
	DXDevice::setSwapInterval(interval);
#elif USE_OPENGL
	glfwSwapInterval(interval);
#endif // USE_DIRECTX
}

void Window::toggleFullscreen(){
	settings->fullscreen = !settings->fullscreen;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (Events::_cursor_locked) Events::toggleCursor();

	if (settings->fullscreen) {
		glfwGetWindowPos(window, &posX, &posY);
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
	}
	else {
		glfwSetWindowMonitor(window, nullptr, posX, posY, settings->width, settings->height, GLFW_DONT_CARE);
		glfwSetWindowAttrib(window, GLFW_MAXIMIZED, GLFW_FALSE);
	}

	double xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);
    Events::setPosition(xPos, yPos);
}

bool Window::isFullscreen() {
	return settings->fullscreen;
}

void Window::swapBuffers(){
#ifdef USE_DIRECTX
	DXDevice::display();
#elif USE_OPENGL
	glfwSwapBuffers(window);
#endif // USE_DIRECTX
	Window::resetScissor();
}

double Window::time() {
	return glfwGetTime();
}

DisplaySettings* Window::getSettings() {
	return settings;
}

ImageData* Window::takeScreenshot() {
#ifdef USE_DIRECTX
	ubyte* data = new ubyte[width * height * 4];
	auto texture = DXDevice::getSurface();
	ID3D11Texture2D* staged = nullptr;
	TextureUtil::stageTexture(texture.Get(), &staged);
	TextureUtil::readPixels(staged, data);
	staged->Release();
	return new ImageData(ImageFormat::rgba8888, width, height, data); 
#elif USE_OPENGL
	ubyte* data = new ubyte[width * height * 3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
	return new ImageData(ImageFormat::rgb888, width, height, data);
#endif // USE_DIRECTX
}

const char* Window::getClipboardText() {
    return glfwGetClipboardString(window);
}

bool Window::tryToMaximize(GLFWwindow* window, GLFWmonitor* monitor) {
	glm::ivec4 windowFrame(0);
	glm::ivec4 workArea(0);
	glfwGetWindowFrameSize(window, &windowFrame.x, &windowFrame.y, &windowFrame.z, &windowFrame.w);
	glfwGetMonitorWorkarea(monitor, &workArea.x, &workArea.y, &workArea.z, &workArea.w);
	if (Window::width > (uint)workArea.z) Window::width = (uint)workArea.z;
	if (Window::height > (uint)workArea.w) Window::height = (uint)workArea.w;
	if (Window::width >= (uint)(workArea.z - (windowFrame.x + windowFrame.z)) &&
		Window::height >= (uint)(workArea.w - (windowFrame.y + windowFrame.w))) {
		glfwMaximizeWindow(window);
		return true;
	}
	glfwSetWindowSize(window, Window::width, Window::height);
	glfwSetWindowPos(window, workArea.x + (workArea.z - Window::width) / 2, 
							 workArea.y + (workArea.w - Window::height) / 2 + windowFrame.y / 2);
	return false;
}