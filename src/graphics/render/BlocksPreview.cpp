#include "BlocksPreview.hpp"

#include "assets/Assets.hpp"
#include "constants.hpp"
#include "content/Content.hpp"
#include "frontend/ContentGfxCache.hpp"
#include "voxels/Block.hpp"
#include "window/Camera.hpp"
#include "window/Window.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/core/Batch3D.hpp"
#include "graphics/core/Framebuffer.hpp"
#include "graphics/core/DrawContext.hpp"
#include "graphics/core/Shader.hpp"
#include "graphics/core/Texture.hpp"
#include "graphics/core/Viewport.hpp"

#include <glm/ext.hpp>

std::unique_ptr<ImageData> BlocksPreview::draw(
    const ContentGfxCache* cache,
    Shader* shader,
    Framebuffer* fbo,
    Batch3D* batch,
    const Block& def, 
    int size
){
    Window::clear();
    blockid_t id = def.rt.id;
    const UVRegion texfaces[6]{cache->getRegion(id, 0), cache->getRegion(id, 1),
                               cache->getRegion(id, 2), cache->getRegion(id, 3),
                               cache->getRegion(id, 4), cache->getRegion(id, 5)};

    glm::vec3 offset(0.1f, 0.5f, 0.1f);
    switch (def.model) {
        case BlockModel::none:
            // something went wrong...
            break;
        case BlockModel::block:
            shader->uniformMatrix("u_apply", glm::translate(glm::mat4(1.0f), offset));
            batch->blockCube(glm::vec3(size * 0.63f), texfaces, 
                             glm::vec4(1.0f), !def.rt.emissive);
            batch->flush();
            break;
        case BlockModel::aabb:
            {
                glm::vec3 hitbox {};
                for (const auto& box : def.hitboxes) {
                    hitbox = glm::max(hitbox, box.size());
                }
                offset = glm::vec3(1, 1, 0.0f);
                shader->uniformMatrix("u_apply", glm::translate(glm::mat4(1.0f), offset));
                glm::vec3 scaledSize = glm::vec3(size * 0.63f);
                batch->cube(
                    -hitbox * scaledSize * 0.5f * glm::vec3(1,1,-1),
                    hitbox * scaledSize,
                    texfaces, glm::vec4(1.0f), 
                    !def.rt.emissive
                );
            }
            batch->flush();
            break;
        case BlockModel::custom:{
            glm::vec3 pmul = glm::vec3(size * 0.63f);
            glm::vec3 hitbox = glm::vec3(1.0f);
            glm::vec3 poff = glm::vec3(0.0f, 0.0f, 1.0f);
            offset.y += (1.0f - hitbox).y * 0.5f;
            shader->uniformMatrix("u_apply", glm::translate(glm::mat4(1.0f), offset));
            const auto& model = cache->getModel(def.rt.id);
            
            for (const auto& mesh : model.meshes) {
                for (const auto& vertex : mesh.vertices) {
                    float d = glm::dot(glm::normalize(vertex.normal), glm::vec3(0.2, 0.8, 0.4));
                    d = 0.8f + d * 0.2f;
                    batch->vertex((vertex.coord - poff)*pmul, vertex.uv, glm::vec4(d, d, d, 1.0f));
                }
                batch->flush();
            }
            break;
        }
        case BlockModel::xsprite: {
            shader->uniformMatrix("u_apply", glm::translate(glm::mat4(1.0f), offset));
            glm::vec3 right = glm::normalize(glm::vec3(1.f, 0.f, -1.f));
            batch->sprite(
                right*float(size)*0.43f+glm::vec3(0, size*0.4f, 0), 
                glm::vec3(0.f, 1.f, 0.f), 
                right, 
                size*0.5f, size*0.6f, 
                texfaces[0], 
                glm::vec4(1.0f)
            );
            batch->flush();
            break;
        }
    }
    return fbo->getTexture()->readData();
}

std::unique_ptr<Atlas> BlocksPreview::build(
    const ContentGfxCache* cache,
    Assets* assets, 
    const Content* content
) {
    auto indices = content->getIndices();
    size_t count = indices->blocks.count();
    size_t iconSize = ITEM_ICON_SIZE;

    auto shader = assets->get<Shader>("ui3d");
    auto atlas = assets->get<Atlas>("blocks");

    Viewport viewport(iconSize, iconSize);
    DrawContext pctx(nullptr, viewport, nullptr);
    DrawContext ctx = pctx.sub();
    ctx.setCullFace(true);
    ctx.setDepthTest(true);

    Framebuffer fbo(iconSize, iconSize, true);
    Batch3D batch(1024);
    batch.begin();

    shader->use();
    shader->uniformMatrix("u_projview",
        glm::ortho(0.0f, float(iconSize), 0.0f, float(iconSize), 
                    -100.0f, 100.0f) * 
        glm::lookAt(glm::vec3(0.57735f), 
                    glm::vec3(0.0f), 
                    glm::vec3(0, 1, 0)));

    AtlasBuilder builder;
    Window::viewport(0, 0, iconSize, iconSize);
    Window::setBgColor(glm::vec4(0.0f));
    
    fbo.bind();
    for (size_t i = 0; i < count; i++) {
        auto& def = indices->blocks.require(i);
        atlas->getTexture()->bind();
        builder.add(def.name, draw(cache, shader, &fbo, &batch, def, iconSize));
    }
    fbo.unbind();

    Window::viewport(0, 0, Window::width, Window::height);
    return builder.build(2);
}
