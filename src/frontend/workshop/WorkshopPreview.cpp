#include "WorkshopPreview.hpp"

#include "assets/AssetsLoader.hpp"
#include "content/Content.hpp"
#include "engine.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/core/DrawContext.hpp"
#include "graphics/core/Shader.hpp"
#include "graphics/core/Texture.hpp"
#include "graphics/core/Viewport.hpp"
#include "graphics/ui/gui_xml.hpp"
#include "items/ItemDef.hpp"
#include "voxels/Block.hpp"
#include "voxels/Chunk.hpp"
#include "voxels/ChunksStorage.hpp"
#include "window/Events.hpp"
#include "window/Window.hpp"
#include "world/Level.hpp"
#include "world/World.hpp"
#include "../ContentGfxCache.hpp"
#include "files/WorldFiles.hpp"
#include "WorkshopUtils.hpp"
#include "maths/rays.hpp"
#include "objects/EntityDef.hpp"
#include "objects/rigging.hpp"

#include <cstring>
#include <glm/gtx/intersect.hpp>

using namespace workshop;

Preview::Preview(Engine* engine, ContentGfxCache* cache) : engine(engine), cache(cache),
blockRenderer(32768, engine->getContent(), cache, &engine->getSettings()),
chunk(new Chunk(0, 0)),
world(new World(WorldInfo(), std::make_shared<WorldFiles>(""), engine->getContent(), engine->getContentPacks())),
level(new Level(std::unique_ptr<World>(world), engine->getContent(), engine->getSettings())),
camera(glm::vec3(0.f), glm::radians(60.f)),
framebuffer(0, 0, true),
inventory(std::make_shared<Inventory>(0, 0)),
cameraPosition(0.f),
lineBatch(1024),
batch2d(1024),
batch3d(1024),
chunks(CHUNK_W, CHUNK_D, 0, 0, nullptr, level),
modelBatch(8192, engine->getAssets(), &chunks, &engine->getSettings()),
mesh(nullptr),
primitiveType(PrimitiveType::COUNT),
lookAtPrimitive(PrimitiveType::COUNT)
{
	level->chunksStorage->store(std::shared_ptr<Chunk>(chunk));
	chunks.putChunk(level->chunksStorage->get(0, 0));
	memset(chunk->voxels, 0, sizeof(chunk->voxels));
	memset(chunk->lightmap.map, std::numeric_limits<light_t>::max(), sizeof(chunk->lightmap.map));
}

Preview::~Preview() {
	delete world;
}

void Preview::update(float delta, float sensitivity) {
	glm::vec3 offset(0.f);
	if (!lockedKeyboardInput) {
		const bool lctrl = Events::pressed(keycode::LEFT_CONTROL);
		const float rotateFactor = (100.f * delta) /** sensitivity*/;
		if (Events::pressed(keycode::D)) {
			if (lctrl) offset.x -= 2.f;
			else rotate(rotateFactor, 0.f);
		}
		if (Events::pressed(keycode::A)) {
			if (lctrl) offset.x += 2.f;
			else rotate(-rotateFactor, 0.f);
		}
		if (Events::pressed(keycode::S)) {
			if (lctrl) offset.z -= 2.f;
			else rotate(0.f, rotateFactor);
		}
		if (Events::pressed(keycode::W)) {
			if (lctrl) offset.z += 2.f;
			else rotate(0.f, -rotateFactor);
		}
		offset = camera.rotation * glm::vec4(offset.x, offset.y, offset.z, 1.f);
		if (Events::pressed(keycode::SPACE)) scale(2.f * delta);
		if (Events::pressed(keycode::LEFT_SHIFT)) scale(-2.f * delta);
	}
	if (lmb) {
		rotate((-Events::delta.x / 2.f) * sensitivity, (Events::delta.y / 2.f) * sensitivity);
		if (!Events::clicked(mousecode::BUTTON_1)) lmb = false;
	}
	if (rmb) {
		offset = camera.rotation * glm::vec4(Events::delta.x, Events::delta.y, 0.f, 1.f);
		if (!Events::clicked(mousecode::BUTTON_2)) rmb = false;
	}
	if (currentUI) {
		refillTimer += delta;
		if (refillTimer > 1.f) {
			refillInventory();
		}
	}
	cameraPosition += (offset / 100.f) * sensitivity;
	cameraPosition.x = std::clamp(cameraPosition.x, -5.f, 5.f);
	cameraPosition.y = std::clamp(cameraPosition.y, -5.f, 5.f);
	cameraPosition.z = std::clamp(cameraPosition.z, -5.f, 5.f);

	camera.rotation = glm::mat4(1.f);
	camera.rotate(glm::radians(previewRotation.y), glm::radians(previewRotation.x), 0);
	camera.position = camera.front * viewDistance;
	camera.position += cameraOffset + cameraPosition;
	camera.dir *= -1.f;
	camera.front *= -1.f;
}

void Preview::updateMesh() {
	if (currentBlock == nullptr) return;
	const bool rotatable = currentBlock->rotatable;
	currentBlock->rotatable = false;
	mesh = blockRenderer.render(chunk, level->chunksStorage.get());
	currentBlock->rotatable = rotatable;
}

void Preview::updateCache() {
	cache->refresh(engine->getContent(), engine->getAssets());
	updateMesh();
}

void Preview::setBlock(Block* block) {
	currentBlock = block;
	chunk->voxels[CHUNK_D * CHUNK_W + CHUNK_D + 1].id = block->rt.id;
	updateMesh();
}

static void updateTextures(rigging::Bone& bone, Assets* assets) {
	if (!bone.model.name.empty()) {
		if (bone.model.model == nullptr) bone.model.refresh(assets);
		if (bone.model.model != nullptr) {
			for (auto& mesh : bone.model.model->meshes) {
				if (mesh.texture.empty() || (!mesh.texture.empty() && mesh.texture.at(0) == '$')) mesh.texture = "blocks:notfound";
			}
		}
	};
	for (auto& subBone : bone.getSubnodes()) {
		updateTextures(*subBone, assets);
	}
}
void Preview::setSkeleton(const rigging::SkeletonConfig* skeleton) {
	currentSkeleton = skeleton;
	if (currentSkeleton) updateTextures(*currentSkeleton->getRoot(), engine->getAssets());
}

void Preview::setModel(model::Model* model) {
	currentModel = model;
	for (auto& mesh : currentModel->meshes) {
		if (mesh.texture.empty() || (!mesh.texture.empty() && mesh.texture.at(0) == '$')) mesh.texture = "blocks:notfound";
	}
}

void Preview::setCurrentAABB(const AABB& aabb, PrimitiveType type) {
	primitiveType = type;
	AABB& aabb_ = (type == PrimitiveType::AABB ? currentAABB : currentHitbox);
	aabb_.a = aabb.a + (aabb.b - aabb.a) / 2.f - 0.5f;
	aabb_.b = aabb.b - aabb.a;
}

void Preview::setCurrentTetragon(const glm::vec3* tetragon) {
	primitiveType = PrimitiveType::TETRAGON;
	for (size_t i = 0; i < 4; i++) {
		currentTetragon[i] = tetragon[i] - 0.5f;
	}
}

void Preview::setUiDocument(const std::shared_ptr<xml::Document> document, std::shared_ptr<int> enviroment, bool forceUpdate) {
	if (document == currentDocument && !forceUpdate) return;
	currentDocument = document;
	AssetsLoader loader(engine->getAssets(), engine->getResPaths());
	gui::UiXmlReader reader(enviroment);
	xml::xmlelement root = document->getRoot();
	currentUI = reader.readXML("", root);

	std::shared_ptr<gui::InventoryView> inventoryView = std::dynamic_pointer_cast<gui::InventoryView>(currentUI);
	if (inventoryView) {
		size_t slotsTotal = inventoryView->getSlotsCount();
		for (const auto& elem : root->getElements()) {
			const std::string& tag = elem->getTag();
			if (tag == "slots-grid") {
				size_t startIndex = 0;
				if (elem->has("start-index")) startIndex = elem->attr("start-index").asInt();
				if (elem->has("count")) slotsTotal = std::max(slotsTotal, startIndex + elem->attr("count").asInt());
				else if (elem->has("rows") && elem->has("cols")) {
					slotsTotal = std::max(slotsTotal, startIndex + elem->attr("rows").asInt() * elem->attr("cols").asInt());
				}
			}
			else if (tag == "slot") {
				if (elem->has("index")) slotsTotal = std::max(slotsTotal, static_cast<size_t>(elem->attr("index").asInt() + 1));
			}
		}
		if (!inventory || inventory->size() != slotsTotal) {
			inventory = std::make_shared<Inventory>(0, slotsTotal);
			refillInventory();
		}
		inventoryView->bind(inventory, engine->getContent());
	}
}

void Preview::setResolution(uint width, uint height) {
	windowWidth = width;
	windowHeight = height;
	width = static_cast<float>(width) * Window::height / height;
	height = Window::height;
	framebuffer.resize(width, height);
	camera.aspect = static_cast<float>(width) / height;
}

void Preview::setBlockSize(const glm::i8vec3& size) {
	blockSize = size;
	cameraOffset = (glm::vec3(size) + glm::vec3(0.f)) * 0.5f - 0.5f;
}

Texture* Preview::getTexture() {
	return framebuffer.getTexture();
}

void Preview::refillInventory() {
	refillTimer = 0.f;
	if (!inventory) return;
	for (size_t i = 0; i < inventory->size(); i++) {
		const ContentIndices* const indices = cache->getContent()->getIndices();
		const ItemDef* const item = indices->items.get(1 + (rand() % (indices->items.count() - 1)));
		if (!item) continue;
		inventory->getSlot(i).set(ItemStack(item->rt.id, rand() % item->stackSize));
	}
}

bool Preview::rayCast(float cursorX, float cursorY, size_t& returnIndex) {
	if (currentBlock) {
		cursorX *= static_cast<float>(framebuffer.getWidth()) / windowWidth;
		cursorY *= static_cast<float>(framebuffer.getHeight()) / windowHeight;

		glm::vec3 dir = glm::unProject(glm::vec3(cursorX, framebuffer.getHeight() - cursorY, 1.f), camera.getView(),
			camera.getProjection(), glm::vec4(0.f, 0.f, framebuffer.getWidth(), framebuffer.getHeight()));

		Ray ray(camera.position + 0.5f, glm::normalize(dir));

		scalar_t distanceMin = std::numeric_limits<scalar_t>::max();
		glm::ivec3 normal;
		//for (const auto& box : currentBlock->modelBoxes) {
		//	scalar_t distance = 0.f;
		//	if (ray.intersectAABB(glm::dvec3(0.0), box, 100.f, normal, distance) > RayRelation::None && distance < distanceMin) {
		//		lookAtPrimitive = PrimitiveType::AABB;
		//		lookAtAABB.a = box.a + (box.b - box.a) / 2.f - 0.5f;
		//		lookAtAABB.b = box.b - box.a;
		//		returnIndex = &box - &currentBlock->modelBoxes.front();
		//		distanceMin = distance;
		//	}
		//}
		//for (size_t i = 0; i < currentBlock->modelExtraPoints.size(); i += 4) {
		//	const glm::vec3* const elem = &currentBlock->modelExtraPoints[i];
		//	glm::vec2 distance(std::numeric_limits<float>::max());
		//	glm::vec2 pos;
		//	if ((glm::intersectRayTriangle(glm::vec3(ray.origin), glm::vec3(ray.dir), elem[0], elem[1], elem[2], pos, distance.x) ||
		//		glm::intersectRayTriangle(glm::vec3(ray.origin), glm::vec3(ray.dir), elem[0], elem[2], elem[3], pos, distance.y)) &&
		//		std::min(distance.x, distance.y) < distanceMin) {
		//		lookAtPrimitive = PrimitiveType::TETRAGON;
		//		for (size_t j = 0; j < 4; j++) {
		//			lookAtTetragon[j] = elem[j] - 0.5f;
		//		}
		//		returnIndex = i / 4;
		//		distanceMin = std::min(distance.x, distance.y);
		//	}
		//}
	}
	return lookAtPrimitive == PrimitiveType::AABB || lookAtPrimitive == PrimitiveType::TETRAGON;
}

void Preview::rotate(float x, float y) {
	previewRotation.x += x;
	previewRotation.y = std::clamp(previewRotation.y + y, -89.f, 89.f);
}

void Preview::scale(float value) {
	viewDistance = std::clamp(viewDistance + value, 1.f, 5.f);
}

void Preview::drawBlock() {
	DrawContext* ctx = beginRenderer(true, true);

	const Assets* const assets = engine->getAssets();
	Texture* const texture = assets->get<Atlas>("blocks")->getTexture();

	auto drawTetragon = [lb = &lineBatch](const glm::vec3* const tetragon, const glm::vec4& color) {
		for (size_t i = 0; i < 4; i++) {
			const size_t next = (i + 1 < 4 ? i + 1 : 0);
			lb->line(tetragon[i], tetragon[next], color);
		}
	};

	Shader* main = setupBlocksShader(glm::vec3(-1.f));
	drawDirectionArrow();
	drawGridLines();

	if (drawBlockSize) lineBatch.box(cameraOffset, glm::vec3(blockSize), glm::vec4(1.f));
	if (lookAtPrimitive == PrimitiveType::AABB) lineBatch.box(lookAtAABB.a, lookAtAABB.b, glm::vec4(1.f));
	else if (lookAtPrimitive == PrimitiveType::TETRAGON) drawTetragon(lookAtTetragon, glm::vec4(1.f));
	if (drawBlockHitbox && primitiveType == PrimitiveType::HITBOX) lineBatch.box(currentHitbox.a, currentHitbox.b, glm::vec4(1.f, 1.f, 0.f, 1.f));
	if (drawCurrentAABB && primitiveType == PrimitiveType::AABB) lineBatch.box(currentAABB.a, currentAABB.b, glm::vec4(1.f, 0.f, 1.f, 1.f));
	if (drawCurrentTetragon && primitiveType == PrimitiveType::TETRAGON) drawTetragon(currentTetragon, glm::vec4(1.f, 0.f, 1.f, 1.f));

	lineBatch.flush();

	main->use();
	texture->bind();
	mesh->draw();
	endRenderer(ctx, false, false);
}

void Preview::drawUI() {
	if (!currentUI) return;
	DrawContext* ctx = beginRenderer(false, false);

	batch2d.begin();

	currentUI->draw(ctx, engine->getAssets());

	endRenderer(ctx, false, false);
}

void Preview::drawSkeleton() {
	DrawContext* ctx = beginRenderer(true, true);

	Shader* const main = setupEntitiesShader(glm::vec3(0.f));

	drawGridLines();
	drawDirectionArrow();

	main->use();
	if (currentSkeleton) {
		rigging::Skeleton skeleton = currentSkeleton->instance();
		currentSkeleton->render(engine->getAssets(), modelBatch, skeleton, glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)));
		modelBatch.render();
	}
	endRenderer(ctx, false, false);
}

void Preview::drawModel() {
	DrawContext* ctx = beginRenderer(true, true);

	Shader* const main = setupEntitiesShader(glm::vec3(0.f));

	drawGridLines();
	drawDirectionArrow();

	main->use();
	if (currentModel) {
		modelBatch.draw(glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)), glm::vec3(1.f), currentModel, nullptr);
		modelBatch.render();
	}

	endRenderer(ctx, false, false);
}

DrawContext* Preview::beginRenderer(bool depthTest, bool cullFace) {
	Window::viewport(0, 0, framebuffer.getWidth(), framebuffer.getHeight());
	framebuffer.bind();
	Window::setBgColor(glm::vec4(0.f));
	Window::clear();
	const Viewport viewport(Window::width, Window::height);
	DrawContext* ctx = new DrawContext(nullptr, viewport, &batch2d);
	ctx->setDepthTest(depthTest);
	ctx->setCullFace(cullFace);
	return ctx;
}

void Preview::endRenderer(DrawContext* context, bool depthTest, bool cullFace) {
	Window::viewport(0, 0, Window::width, Window::height);
	context->setDepthTest(depthTest);
	context->setCullFace(cullFace);
	framebuffer.unbind();
	delete context;
}

Shader* Preview::setupShader(const std::string& name, const glm::vec3& offset) {
	Shader* const shader = engine->getAssets()->get<Shader>(name);
	//glm::mat4 proj = glm::ortho(-1.f, 1.f, -1.f, 1.f, -100.0f, 100.0f);
	//glm::mat4 view = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0.5f), glm::vec3(0, 1, 0));
	shader->use();
	shader->uniformMatrix("u_model", glm::translate(glm::mat4(1.f), glm::vec3(offset)));
	shader->uniformMatrix("u_proj", camera.getProjection());
	shader->uniformMatrix("u_view", camera.getView());
	shader->uniform1f("u_fogFactor", 0.f);
	shader->uniform3f("u_fogColor", glm::vec3(1.f));
	shader->uniform1f("u_fogCurve", 100.f);
	shader->uniform3f("u_cameraPos", camera.position);
	//shader->uniform3f("u_torchlightColor", glm::vec3(0.5f));
	//shader->uniform1f("u_torchlightDistance", 100.0f);
	//shader->uniform1f("u_gamma", 0.1f);
	shader->uniform1i("u_cubemap", 1);
	return shader;
}

Shader* Preview::setupEntitiesShader(const glm::vec3& offset) {
	return setupShader("entity", offset);
}

Shader* Preview::setupBlocksShader(const glm::vec3& offset) {
	return setupShader("main", offset);
}

Shader* Preview::drawGridLines() {
	const Assets* const assets = engine->getAssets();
	Shader* const shader = assets->get<Shader>("lines");
	if (!drawGrid) return shader;

	for (float i = -3.f; i < 3; i++) {
		lineBatch.line(glm::vec3(i + 0.5f, -0.5f, -3.f), glm::vec3(i + 0.5f, -0.5f, 3.f), glm::vec4(0.f, 0.f, 1.f, 1.f));
	}
	for (float i = -3.f; i < 3; i++) {
		lineBatch.line(glm::vec3(-3.f, -0.5f, i + 0.5f), glm::vec3(3.f, -0.5f, i + 0.5f), glm::vec4(1.f, 0.f, 0.f, 1.f));
	}

	lineBatch.lineWidth(5.f);
	shader->use();
	shader->uniformMatrix("u_projview", camera.getProjView());
	lineBatch.flush();

	return shader;
}

Shader* Preview::drawDirectionArrow() {
	const Assets* const assets = engine->getAssets();
	Shader* const shader = assets->get<Shader>("ui3d");

	if (!drawDirection) return shader;
	shader->use();
	shader->uniformMatrix("u_apply", glm::mat4(1.f));
	shader->uniformMatrix("u_projview", glm::translate(camera.getProjView(), glm::vec3(cameraOffset.x - 1.f, -0.98f, blockSize.z - 2.f)));

	batch3d.begin();
	batch3d.sprite(glm::vec3(1.f, 0.5f, 1.8f), glm::vec3(0.2f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.2f), 1.f, 1.f, UVRegion(), glm::vec4(0.8f));
	batch3d.point(glm::vec3(1.f, 0.5f, 2.4f), glm::vec4(0.8f));
	batch3d.point(glm::vec3(1.3f, 0.5f, 2.f), glm::vec4(0.8f));
	batch3d.point(glm::vec3(0.7f, 0.5f, 2.f), glm::vec4(0.8f));
	batch3d.flush();

	return shader;
}
