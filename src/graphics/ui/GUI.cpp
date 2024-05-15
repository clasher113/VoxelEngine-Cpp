#include "GUI.hpp"
#include "elements/UINode.hpp"
#include "elements/Menu.hpp"

#include "../../assets/Assets.hpp"
#include "../../frontend/UiDocument.hpp"
#include "../../graphics/core/Batch2D.hpp"
#include "../../graphics/core/Shader.hpp"
#include "../../graphics/core/DrawContext.hpp"
#include "../../window/Events.hpp"
#include "../../window/Window.hpp"
#include "../../window/input.hpp"
#include "../../window/Camera.hpp"

#include <iostream>
#include <algorithm>

using namespace gui;

GUI::GUI() {
    container = std::make_shared<Container>(glm::vec2(1000));
    uicamera = std::make_unique<Camera>(glm::vec3(), Window::height);
    uicamera->perspective = false;
    uicamera->flipped = true;

    menu = std::make_shared<Menu>();
    menu->setId("menu");
    container->add(menu);
    container->setScrollable(false);
}

GUI::~GUI() {
}

std::shared_ptr<Menu> GUI::getMenu() {
    return menu;
}

void GUI::onAssetsLoad(Assets* assets) {
    assets->store(new UiDocument(
        "core:root", 
        uidocscript {}, 
        std::dynamic_pointer_cast<gui::UINode>(container), 
        nullptr
    ), "core:root");
}

// @brief Mouse related input and logic handling 
void GUI::actMouse() {
    auto hover = container->getAt(Events::cursor, nullptr);
    if (this->hover && this->hover != hover) {
        this->hover->setHover(false);
    }
    if (hover) {
        hover->setHover(true);
        if (Events::scroll) {
            hover->scrolled(Events::scroll);
        }
    }
    this->hover = hover;

    if (Events::jclicked(mousecode::BUTTON_1)) {
        if (pressed == nullptr && this->hover) {
            pressed = hover;
            pressed->click(this, Events::cursor.x, Events::cursor.y);
            if (focus && focus != pressed) {
                focus->defocus();
            }
            if (focus != pressed) {
                focus = pressed;
                focus->onFocus(this);
                return;
            }
        }
        if (this->hover == nullptr && focus) {
            focus->defocus();
            focus = nullptr;
        }
    } else if (pressed) {
        pressed->mouseRelease(this, Events::cursor.x, Events::cursor.y);
        pressed = nullptr;
    }

    if (hover) {
        for (mousecode code : MOUSECODES_ALL) {
            if (Events::jclicked(code)) {
                hover->clicked(this, code);
            }
        }
    }
} 

void GUI::actFocused() {
    if (Events::jpressed(keycode::ESCAPE)) {
        focus->defocus();
        focus = nullptr;
        return;
    }
    for (auto codepoint : Events::codepoints) {
        focus->typed(codepoint);
    }
    for (auto key : Events::pressedKeys) {
        focus->keyPressed(key);
    }

    if (!Events::_cursor_locked) {
        if (Events::clicked(mousecode::BUTTON_1) && 
            (Events::jclicked(mousecode::BUTTON_1) || Events::delta.x || Events::delta.y))
        {
            focus->mouseMove(this, Events::cursor.x, Events::cursor.y);
        }
    }
}

void GUI::act(float delta, const Viewport& vp) {
    while (!postRunnables.empty()) {
        runnable callback = postRunnables.back();
        postRunnables.pop();
        callback();
    }

    container->setSize(vp.size());
    container->act(delta);
    auto prevfocus = focus;

    if (!Events::_cursor_locked) {
        actMouse();
    }
    
    if (focus) {
        actFocused();
    }
    if (focus && !focus->isFocused()) {
        focus = nullptr;
    }
}

void GUI::draw(const DrawContext* pctx, Assets* assets) {
    auto& viewport = pctx->getViewport();
    glm::vec2 wsize = viewport.size();

    menu->setPos((wsize - menu->getSize()) / 2.0f);
    uicamera->setFov(wsize.y);

    Shader* uishader = assets->getShader("ui");
    uishader->use();
    uishader->uniformMatrix("u_projview", uicamera->getProjection()*uicamera->getView());

    pctx->getBatch2D()->begin();
    container->draw(pctx, assets);
}

std::shared_ptr<UINode> GUI::getFocused() const {
    return focus;
}

bool GUI::isFocusCaught() const {
    return focus && focus->isFocuskeeper();
}

void GUI::add(std::shared_ptr<UINode> node) {
    container->add(node);
}

void GUI::remove(std::shared_ptr<UINode> node) noexcept {
    container->remove(node);
}

void GUI::store(std::string name, std::shared_ptr<UINode> node) {
    storage[name] = node;
}

std::shared_ptr<UINode> GUI::get(std::string name) noexcept {
    auto found = storage.find(name);
    if (found == storage.end()) {
        return nullptr;
    }
    return found->second;
}

void GUI::remove(std::string name) noexcept {
    storage.erase(name);
}

void GUI::setFocus(std::shared_ptr<UINode> node) {
    if (focus) {
        focus->defocus();
    }
    focus = node;
    if (focus) {
        focus->onFocus(this);
    }
}

std::shared_ptr<Container> GUI::getContainer() const {
    return container;
}

void GUI::postRunnable(runnable callback) {
    postRunnables.push(callback);
}
