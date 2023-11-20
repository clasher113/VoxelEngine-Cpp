#include "definitions.h"

#include "window/Window.h"
#include "window/Events.h"
#include "window/input.h"
#include "voxels/Block.h"

// All in-game definitions (blocks, items, etc..)
void setup_definitions() {
	for (size_t i = 0; i < 256; i++) {
		Block::blocks[i] = nullptr;
	}

	Block* block = new Block(BLOCK_AIR, 0);
	block->drawGroup = 1;
	block->lightPassing = true;
	block->skyLightPassing = true;
	block->obstacle = false;
	block->selectable = false;
	block->model = BlockModel::none;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_DIRT, 2);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_GRASS_BLOCK, 4);
	block->textureFaces[2] = 2;
	block->textureFaces[3] = 1;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_LAMP, 3);
	block->emission[0] = 15;
	block->emission[1] = 14;
	block->emission[2] = 13;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_GLASS,5);
	block->drawGroup = 2;
	block->lightPassing = true;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_PLANKS, 6);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_WOOD, 7);
	block->textureFaces[2] = 8;
	block->textureFaces[3] = 8;
	block->rotatable = true;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_LEAVES, 9);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_STONE, 10);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_WATER, 11);
	block->drawGroup = 4;
	block->lightPassing = true;
	block->skyLightPassing = false;
	block->obstacle = false;
	block->selectable = false;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_SAND, 12);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_BEDROCK, 13);
	block->breakable = false;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_GRASS, 14);
	block->drawGroup = 5;
	block->lightPassing = true;
	block->obstacle = false;
	block->model = BlockModel::xsprite;
	block->hitboxScale = 0.5f;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_FLOWER, 16);
	block->drawGroup = 5;
	block->lightPassing = true;
	block->obstacle = false;
	block->model = BlockModel::xsprite;
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_BRICK, 17);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_METAL, 18);
	Block::blocks[block->id] = block;

	block = new Block(BLOCK_RUST, 19);
	Block::blocks[block->id] = block;
}

void setup_bindings() {
	Events::bind(BIND_MOVE_FORWARD, inputtype::keyboard, keycode::W);
	Events::bind(BIND_MOVE_BACK, inputtype::keyboard, keycode::S);
	Events::bind(BIND_MOVE_RIGHT, inputtype::keyboard, keycode::D);
	Events::bind(BIND_MOVE_LEFT, inputtype::keyboard, keycode::A);
	Events::bind(BIND_MOVE_JUMP, inputtype::keyboard, keycode::SPACE);
	Events::bind(BIND_MOVE_SPRINT, inputtype::keyboard, keycode::LEFT_CONTROL);
	Events::bind(BIND_MOVE_CROUCH, inputtype::keyboard, keycode::LEFT_SHIFT);
	Events::bind(BIND_MOVE_CHEAT, inputtype::keyboard, keycode::R);
	Events::bind(BIND_CAM_ZOOM, inputtype::keyboard, keycode::C);
	Events::bind(BIND_PLAYER_NOCLIP, inputtype::keyboard, keycode::N);
	Events::bind(BIND_PLAYER_FLIGHT, inputtype::keyboard, keycode::F);
	Events::bind(BIND_HUD_INVENTORY, inputtype::keyboard, keycode::TAB);
}