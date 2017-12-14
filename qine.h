/*
 * qine.h
 *
 *  Created on: 8 jul 2011
 *      Author: o40
 */

#ifndef QINE_H_
#define QINE_H_

#define WORLD_X 256
#define WORLD_Y 256
#define WORLD_Z 64

#include <vector>
#include <fstream>
#include <iostream>
#include <list>

using namespace std;

enum optimizeBy {
		optimizeByX = 0,
		optimizeByY,
		optimizeByZ
	};

namespace qine {

class qine {

	enum blockType {
		Air = 0,
		Stone,
		Grass,
		Dirt,
		Cobblestone,
		WoodenPlank,
		Sapling,
		Bedrock,
		Water,
		StationaryWater,
		Lava, // 0x0a
		StationaryLava,
		Sand,
		Gravel,
		GoldOre,
		IronOre, // 0x0f
		CoalOre,
		Wood,
		Leaves,
		Sponge,
		Glass,
		LapisLazuliOre,
		LapisLazuliBlock,
		Dispenser,
		Sandstone,
		NoteBlock,
		Bed,
		PoweredRail,
		DetectorRail,
		StickyPiston,
		Cobweb,
		TallGrass,
		DeadShrubs,
		Piston,
		PistonExtension,
		Wool,
		BlockMovedByPiston,
		Dandelion,
		Rose,
		BrownMushroom,
		RedMushroom,
		GoldBlock,
		IronBlock,
		DoubleSlabs,
		Slabs,
		BrickBlock,
		TNT,
		Bookshelf,
		MossStone,
		Obsidian,
		Torch,
		Fire,
		MonsterSpawner,
		WoodenStairs,
		Chest,
		RedstoneWire,
		DiamondOre,
		DiamondBlock,
		CraftingTable,
		Seeds,
		Farmland,
		Furnace,
		BurningFurnace,
		SignPost,
		WoodenDoor,
		Ladders,
		Rails,
		CobblestoneStairs,
		WallSign,
		Lever,
		StonePressurePlate,
		IronDoor,
		WoodenPressurePlate,
		RedstoneOre,
		GlowingRedstoneOre,
		RedstoneTorchOff,
		RedstoneTorchOn,
		StoneButton,
		Snow,
		Ice,
		SnowBlock,
		Cactus,
		ClayBlock,
		SugarCane,
		Jukebox,
		Fence,
		Pumpkin,
		Netherrack,
		SoulSand,
		GlowstoneBlock,
		Portal,
		JackOLantern,
		CakeBlock,
		RedstoneRepeaterOff,
		RedstoneRepeaterOn,
		LockedChest,
		Trapdoor,

		Hint = 0xffff
	};

	enum direction { // aka what side to texture
		zp  = 1<<0, // 1 Z+
		zm  = 1<<1, // 2 Z- etc...
		xp  = 1<<2, // 4
		xm  = 1<<3, // 8
		yp  = 1<<4, // 16
		ym  = 1<<5  // 32
	};



	struct block {
		int x;
		int y;
		int z;
		int width;
		int length;
		int height;
		int type;

		block(int x,int y,int z, int type) : x(x), y(y), z(z), type(type), width(1), height(1), length(1) {};
	};

	struct hintBrush {
		int x;
		int y;
		int z;
		int width;  // x-axis
		int length; // y-axis
		int height; // z-axis

		bool markedForDeletion;
		bool markedForDeletion2;

		hintBrush() {};
		hintBrush(int x, int y, int z, int width, int length, int height) :
			x(x), y(y), z(z), width(width), length(length), height(height),
			markedForDeletion(false), markedForDeletion2(false){};



	};

	struct position {
		int x;
		int y;
		int z;
		int direction;
		char lastType;
		position(int x, int y, int z, int dir, char lastType) :
			x(x), y(y), z(z), direction(dir), lastType(lastType) {};
 	};

	struct texturing {
		bool top;
		bool bot;
		bool lft;
		bool rgt;
		bool bck;
		bool fnt;

		texturing() : top(false), bot(false), lft(false), rgt(false), bck(false), fnt(false) {};
		texturing(bool b) : top(b), bot(b), lft(b), rgt(b), bck(b), fnt(b) {};
	};

public:
	struct mapBlock {
		block blck;
		int texturing;
		mapBlock() : blck(0,0,0,0), texturing(0) {};
		mapBlock(block blk, int tex) : blck(blk), texturing(tex) {};
	};

public:
	qine(std::string datname, int width, int height, int hintSize);
	virtual ~qine();

	void filterBlocks();
	int createBlockList();
	void createMapFile(std::string);

	void createBrush(int x, int y, int z, int length, int y_length, int height, int type, int texturing);

	void checkBlockList();

	void createHints();
	void MergeHints();
	void markHintForDeletion(int x, int y, int z);
	void removeUselessHints();

	int Optimize(int direction);

	void removeUncheckedBlocks();
	void printLayer(int z, int size);

	int blockCount();

private:
	vector<mapBlock> m_BlockCollection;
	vector < vector < vector<hintBrush> > > m_Hint3dArray;

	vector < vector < vector<int> > > m_Blocks;
	vector < vector < vector<int> > > m_TempBlocks;
	vector < vector < vector<int> > > m_CheckList;
	vector < vector < vector<int> > > m_TextureList;

	int m_OffsetX;
	int m_OffsetY;

	int m_Width;  // x
	int m_Length; // y

	int m_HintSize;

	struct MapBlockComparatorX {
		bool operator()(const mapBlock & first, const mapBlock & second) {
			return first.blck.x < second.blck.x;
		}
	} mapBlockComparatorX;

	struct MapBlockComparatorY {
		bool operator()(const mapBlock & first, const mapBlock & second) {
			return first.blck.y < second.blck.y;
		}
	} mapBlockComparatorY;

	struct MapBlockComparatorZ {
		bool operator()(const mapBlock & first, const mapBlock & second) {
			return first.blck.z > second.blck.z;
		}
	} mapBlockComparatorZ;

	char getBlockAtXYZ(char* arr, int x, int y, int z);
	void setBlockAtXYZ(char* arr, int x, int y, int z, char ch);

	void getTextures(int type, int texturing, string& xp_tex, string& xn_tex, string& yp_tex, string& yn_tex, string& zp_tex, string& zn_tex, int& blockflags);

	bool isSolid(int type);
	bool isDetail(int type);

	int worldSize();

	ofstream m_OutFile;
};

} /* namespace qine */
#endif /* QINE_H_ */
