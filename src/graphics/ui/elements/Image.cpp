#include "Image.hpp"

#include "../../core/DrawContext.hpp"
#include "../../core/Batch2D.hpp"
#include "../../core/Texture.hpp"
#include "../../../assets/Assets.hpp"
#include "../../../maths/UVRegion.hpp"

using namespace gui;

Image::Image(std::string texture, glm::vec2 size) : UINode(size), texture(texture) {
    setInteractive(false);
}

Image::Image(Texture* texture, glm::vec2 size) : UINode(size), tex(texture) {
    setInteractive(false);
}

void Image::draw(const DrawContext* pctx, Assets* assets) {
    glm::vec2 pos = calcPos();
    auto batch = pctx->getBatch2D();
    
    auto texture = (tex ? tex : assets->getTexture(this->texture));
    if (texture && autoresize) {
        setSize(glm::vec2(texture->getWidth(), texture->getHeight()));
    }
    batch->texture(texture);
    batch->rect(
        pos.x, pos.y, size.x, size.y, 
        0, 0, 0, uv, false, true, calcColor()
    );
}

void Image::setAutoResize(bool flag) {
    autoresize = flag;
}
bool Image::isAutoResize() const {
    return autoresize;
}
