#include "qine.h"
#include <string>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <math.h>
#include <assert.h>

#define MAX_MAP_BRUSHES 32768

using namespace std;

void displayHelp();

int main(int argc, char* argv[])
{
	std::string mapname, datname;

	int x = 100;
	int y = 100;
	int hintSize = 0;

	int c;
	while ((c = getopt(argc, argv, "x:y:o:i:h:")) != -1)
	{
		switch (c)
		{
		case 'x':
			x = atoi(optarg);
			break;
		case 'y':
			y = atoi(optarg);
			break;
		case 'o':
			mapname = optarg;
			break;
		case 'i':
			datname = optarg;
			break;
		case 'h':
			hintSize = atoi(optarg);
			break;
		default:
			displayHelp();
			return 1;
		}
	}

	// Argument requirements
	if (datname.length() == 0)
	{
		cout << "--- ERROR: You need to specify minecraft data file" << endl;
		displayHelp();
		return 1;
	}

	if (mapname.length() == 0)
	{
		cout << "--- ERROR: You need to specify a output map name" << endl;
		displayHelp();
		return 1;
	}

	cout << "Converting world" << endl;
	cout << "X-Size: " << x << endl;
	cout << "Y-Size: " << y << endl;
	cout << "Hint size: " << hintSize << endl;
	cout << "Input filename: " << datname << endl;
	cout << "Output filename: " << mapname << endl << endl;

	// Create map object
	qine::qine qine(datname, x, y, hintSize);

	// Remove all blocks that we do not want
	qine.filterBlocks();

	if (hintSize > 0)
	{
		qine.createHints();
	}

	// Remove blocks we cannot see or reach
	qine.checkBlockList();

	if (hintSize > 0)
	{
		qine.removeUselessHints();
	}

	qine.removeUncheckedBlocks();
	// Create a list of blocks and merge if possible
	qine.createBlockList();

	int opt;
	cout << "optimizing X-axis: " << endl;
	opt = qine.Optimize(optimizeByX);
	cout << setw(4) << opt << " merged blocks in x-axis" << endl;

	cout << "optimizing Y-axis:" << endl;
	opt = qine.Optimize(optimizeByY);
	cout << setw(4) << opt << " merged blocks in y-axis" << endl;

	cout << "optimizing Z-axis: " << endl;
	opt = qine.Optimize(optimizeByZ);
	cout << setw(4) << opt << " merged blocks in z-axis" << endl;

	cout << "There is a total of " << qine.blockCount() << " blocks left in the list" << endl;

	qine.createMapFile(mapname);

	return 0;
}

void displayHelp()
{
	cout << "Arguments:" << endl;
	cout << "-x xsize (amount of blocks in x-axis, default 100)" << endl;
	cout << "-y ysize (amount of blocks in y-axis, default 100)" << endl;
	cout << "-o outputfile (like mineqraft.map)" << endl;
	cout << "-i inputfile (like level.dat)" << endl << endl;
	cout << "-h hint size (for manual hinting)" << endl << endl;
	cout << "Example mineqraft.exe -x 64 -y 64 -i level.dat -o mykewl.map" << endl << endl;
}

namespace qine {

/*
 * Constructor.
 */
qine::qine(std::string datname, int width, int length, int hintSize)
{
	m_OffsetX = 0;
	m_OffsetY = 0;

	m_Width = width;
	m_Length = length;

	m_HintSize = hintSize;

	m_Blocks.resize(m_Width);
	m_TempBlocks.resize(m_Width);
	m_CheckList.resize(m_Width);
	m_TextureList.resize(m_Width);

	for (int i = 0; i < m_Width; ++i)
	{
		m_Blocks[i].resize(m_Length);
		m_TempBlocks[i].resize(m_Length);
		m_CheckList[i].resize(m_Length);
		m_TextureList.resize(m_Length);
		for (int j = 0; j < m_Length; ++j)
		{
			m_Blocks[i][j].resize(WORLD_Z);
			m_TempBlocks[i][j].resize(WORLD_Z);
			m_CheckList[i][j].resize(WORLD_Z);
			m_TextureList[i][j].resize(WORLD_Z);
		}
	}

	/*m_Blocks = new char[worldSize()];
	m_TempBlocks = new char[worldSize()];
	m_CheckList = new char[worldSize()];
	m_TextureList = new char[worldSize()];*/


	int numHintsWidth = ceil(width/hintSize);
	int numHintsLength = ceil(length/hintSize);
	int numHintsHeight = ceil(WORLD_Z/hintSize);

	cout << " A " << numHintsWidth << " A "<< numHintsLength << " A "<< numHintsHeight << endl;

    m_Hint3dArray.resize(numHintsWidth);
    for (int i = 0; i < numHintsWidth; ++i)
	{
		m_Hint3dArray[i].resize(numHintsLength);
		for (int j = 0; j < numHintsLength; ++j)
		{
			m_Hint3dArray[i][j].resize(numHintsHeight);
		}
	}

    cout << " A " << m_Hint3dArray[0][0].size() << " A "<< m_Hint3dArray[0].size() << " A "<< m_Hint3dArray.size() << endl;

	for (int i = 0; i < worldSize(); i++) {
		m_CheckList[i] = 0;
		m_TextureList[i] = 0;
	}

	ifstream file (datname.c_str(), ios::in|ios::binary|ios::ate);

	char* leveldata = new char[worldSize()];

	// Jump to first level data
	file.seekg(0x47bc, ios::beg);
	file.read(leveldata, worldSize());

	file.close();

	// populate world data vector

	// Loop z-axis
	for (int i = 0; i < WORLD_Z; i++)
	{
		// Loop y-axis
		for (int j = 0; j < m_Length; j++)
		{
			// Loop x-axis
			for (int k = 0; k < m_Width; k++)
			{
				m_Blocks[k][j][i] = getBlockAtXYZ(leveldata, k, j, i);
			}
		}
	}
}

/*
 * Destructor
 */
qine::~qine()
{
	delete [] m_Blocks;
	delete [] m_TempBlocks;
	delete [] m_CheckList;
	delete [] m_TextureList;
}

/*
 * Gets block type at x,y,z from supplied array
 */
char qine::getBlockAtXYZ(char* arr, int x, int y, int z)
{
	return arr[x+y*WORLD_X+z*WORLD_X*WORLD_Y];
}

/*
 * Sets block type at x,y,z on supplied array
 */
void qine::setBlockAtXYZ(char* arr, int x, int y, int z, char ch)
{
	arr[x+y*WORLD_X+z*WORLD_X*WORLD_Y] = ch;
}

/*
 * Removes (set to Air) all blocks not listed in this function
 */
void qine::filterBlocks()
{
	int blocksFiltered = 0;

	for (int i = 0; i < worldSize(); i++)
	{
		// create bedrock in the three lowest layers because of problems with lava
		// TODO: Confirm that this is still a problem or if this can be removed
		if (i < ((3*(WORLD_X * WORLD_Y)) -1))
		{
			m_Blocks[i] = Stone;
		}

		else if(!(
				m_Blocks[i] == Stone ||
				m_Blocks[i] == Grass   ||
				m_Blocks[i] == Dirt ||
			 	m_Blocks[i] == Cobblestone ||
			 	m_Blocks[i] == Bedrock ||
				m_Blocks[i] == Water ||
				m_Blocks[i] == StationaryWater ||
				m_Blocks[i] == Lava ||
				m_Blocks[i] == StationaryLava ||
				m_Blocks[i] == Sand ||
			 	m_Blocks[i] == Gravel ||
				m_Blocks[i] == GoldOre ||
				m_Blocks[i] == IronOre ||
				m_Blocks[i] == CoalOre ||
				m_Blocks[i] == Wood ||
				m_Blocks[i] == Leaves ||
				m_Blocks[i] == Sandstone ||
				m_Blocks[i] == Glass ||
				m_Blocks[i] == LapisLazuliOre ||
				m_Blocks[i] == LapisLazuliBlock ||
				m_Blocks[i] == MossStone ||
				m_Blocks[i] == Obsidian ||
				m_Blocks[i] == DiamondOre ||
				m_Blocks[i] == Farmland ||
				m_Blocks[i] == RedstoneOre ||
				m_Blocks[i] == GlowingRedstoneOre ||
				m_Blocks[i] == Snow ||
				m_Blocks[i] == Ice ||
				m_Blocks[i] == SnowBlock ||
				m_Blocks[i] == ClayBlock ||
				m_Blocks[i] == SoulSand ||
				m_Blocks[i] == GlowstoneBlock
		)) {
			if (m_Blocks[i] != Air) blocksFiltered++;
			m_Blocks[i] = Air;
		}
	}
	cout << dec << blocksFiltered << " unwanted \"blocks\" (like flowers) filtered out (" << 100*blocksFiltered/(worldSize()) << " %)" << endl ;
}

/*
 * Removes all blocks that we cannot see from the given x,y,z position
 * using a flood fill algorithm. Also mark hints for deletion.
 */
void qine::checkBlockList()
{
	int i = 0;

	list<position> checkList;

	// Choose a good start position for the flood fill
	int startX = ((m_Width) / 2) + m_OffsetX;
	int startY = ((m_Length) / 2) + m_OffsetY;

	position startPosition(startX, startY , WORLD_Z - 1, 0, 0);

	checkList.push_back(startPosition);

	while (!checkList.empty()) {

		// Pop and delete first item
		position pos = checkList.front();
		checkList.pop_front();

		// Make sure item is in range
		if (
				(pos.x - m_OffsetX) < 0 ||
				pos.x > (WORLD_X - 1) ||
				pos.x > (m_OffsetX + m_Width - 1) ||
				(pos.y - m_OffsetY) < 0 ||
				pos.y > (WORLD_Y - 1) ||
				pos.y > (m_OffsetY + m_Length - 1) ||
				pos.z < 0 ||
				pos.z > (WORLD_Z - 1))

		{
			continue;
		}

		// Get type and take action accordingly
		char type = getBlockAtXYZ(m_Blocks, pos.x, pos.y, pos.z);

		if (!isDetail(type))
		{
			markHintForDeletion(pos.x, pos.y, pos.z);
		}

		if ((isSolid(type))
		//Only tex water that connects with air
		|| ((type == Water || type == StationaryWater
				|| type == Lava || type == StationaryLava) && pos.lastType == Air))
		{
			int tmp = getBlockAtXYZ(m_TextureList, pos.x, pos.y, pos.z);
			tmp |= pos.direction;
			setBlockAtXYZ(m_TextureList, pos.x, pos.y, pos.z, tmp);
		}


		if (getBlockAtXYZ(m_CheckList, pos.x, pos.y, pos.z) == 0)
		{
			setBlockAtXYZ(m_CheckList, pos.x, pos.y, pos.z, 1);
			if (!isDetail(type))
			{
				checkList.push_back(position(pos.x + 1, pos.y, pos.z, xm, type));
				checkList.push_back(position(pos.x - 1, pos.y, pos.z, xp, type));
				checkList.push_back(position(pos.x, pos.y + 1, pos.z, ym, type));
				checkList.push_back(position(pos.x, pos.y - 1, pos.z, yp, type));
				checkList.push_back(position(pos.x, pos.y, pos.z + 1, zm, type));
				checkList.push_back(position(pos.x, pos.y, pos.z - 1, zp, type));
			}
		}
	}
}

/*
 * Returns true if the block type is a solid
 */
bool qine::isSolid(int type)
{
	return !(type == Air ||
			type == Water ||
			type == StationaryWater ||
			type == Lava ||
			type == StationaryLava
			);
}

/*
 * Returns true if the block type should be a detail brush
 */
bool qine::isDetail(int type)
{
	return !(type == Air ||
			type == Water ||
			type == StationaryWater ||
			type == Lava ||
			type == StationaryLava ||
			type == Wood || // To be able to make them detail
			type == Leaves
			);
}


/*
 * Sets all the blocks that are not in m_Checklist to Air
 */
void qine::removeUncheckedBlocks()
{
	int numBlocks = 0;

	for (int i = 0; i < worldSize(); i++) {
		if (m_CheckList[i] == 0) {
			m_Blocks[i] = 0;
			numBlocks++;
		}
	}
}

/*
 * Returns the volume (in blocks) of the world
 */
int qine::worldSize()
{
	return WORLD_X*WORLD_Y*WORLD_Z;
}

/*
 * Creates a list of blocks
 */
int qine::createBlockList()
{
	int mergedBlocks = 0;
	char ch;
	char tex;

	ch = getBlockAtXYZ(m_Blocks, 0,0,0);
	tex = getBlockAtXYZ(m_TextureList, 0,0,0);
	mapBlock currentMapBlock;

	int numblocks = 0;
	for (int p = 0; p < worldSize(); p++) {
		if (m_Blocks[p] != 0)
			numblocks++;
	}
	cout << "There are " << numblocks << " blocks in the list" <<  endl;

	// Loop z-axis
	for (int i = 0; i < WORLD_Z; i++)
	{
		// Loop y-axis
		for (int j = m_OffsetY; j < m_Length; j++)
		{
			// Loop x-axis
			for (int k = m_OffsetX; k < m_Width; k++)
			{
				ch = getBlockAtXYZ(m_Blocks, k, j, i);
				tex = getBlockAtXYZ(m_TextureList, k, j, i);

				currentMapBlock.blck.x = k;
				currentMapBlock.blck.y = j;
				currentMapBlock.blck.z = i;
				currentMapBlock.blck.width = 1;
				currentMapBlock.blck.type = ch;
				currentMapBlock.texturing = tex;
				if (currentMapBlock.blck.type != Air) {
					m_BlockCollection.push_back(currentMapBlock);
				}
			}
		}
	}
	return mergedBlocks;
}

void qine::createHints()
{
	cout << "Creating hints" << endl;

	// Loop z-axis
	for (int z = 0; z < m_Hint3dArray[0][0].size(); z++)
	{
		// Loop y-axis
		for (int y = 0; y < m_Hint3dArray[0].size(); y++)
		{
			// Loop x-axis
			for (int x = 0; x < m_Hint3dArray.size(); x++)
			{
				m_Hint3dArray[x][y][z].x = x * m_HintSize;
				m_Hint3dArray[x][y][z].y = y * m_HintSize;
				m_Hint3dArray[x][y][z].z = z * m_HintSize;

				if (x * m_HintSize + m_HintSize <= m_Width)
				{
					m_Hint3dArray[x][y][z].width = m_HintSize;
				}
				else
				{
					m_Hint3dArray[x][y][z].width = m_Width - x;
				}

				if (y * m_HintSize + m_HintSize <= m_Length)
				{
					m_Hint3dArray[x][y][z].length = m_HintSize;
				}
				else
				{
					m_Hint3dArray[x][y][z].length = m_Length - y;
				}

				m_Hint3dArray[x][y][z].height = m_HintSize;
			}
		}
	}

	cout << "Creating hints done" << endl;
}

void qine::markHintForDeletion(int x, int y, int z)
{
	// Compare to the correct hint
	int hintX = floor(x / m_HintSize);
	int hintY = floor(y / m_HintSize);

	// TODO: Make this properly instead of trial and error
	int hintZ = floor((z+1) / m_HintSize);

	m_Hint3dArray[hintX][hintY][hintZ].markedForDeletion = true;
}

void qine::removeUselessHints()
{
	cout << "Removing useless hints" << endl;

	bool px, nx, py, ny, pz, nz;

	for (int z = 0; z < m_Hint3dArray[0][0].size(); z++)
	{
		for (int y = 0; y < m_Hint3dArray[0].size(); y++)
		{
			for (int x = 0; x < m_Hint3dArray.size(); x++)
			{
				px = false;
				nx = false;
				py = false;
				ny = false;
				pz = false;
				nz = false;
				// Check surrounding blocks

				// +x
				if ((x == (m_Hint3dArray.size() - 1))
						|| !m_Hint3dArray[x + 1][y][z].markedForDeletion)
				{
					px = true;
				}

				// -x
				if (x == 0 || !m_Hint3dArray[x - 1][y][z].markedForDeletion)
				{
					nx = true;
				}

				// +y
				if ((y == (m_Hint3dArray.size() - 1))
						|| !m_Hint3dArray[x][y + 1][z].markedForDeletion)
				{
					py = true;
				}

				// -y
				if (y == 0 || !m_Hint3dArray[x][y - 1][z].markedForDeletion)
				{
					ny = true;
				}

				// +z
				if ((z == (m_Hint3dArray.size() - 1))
						|| !m_Hint3dArray[x][y][z + 1].markedForDeletion)
				{
					pz = true;
				}

				// -z
				if (z == 0 || !m_Hint3dArray[x][y][z - 1].markedForDeletion)
				{
					nz = true;
				}

				if (px && nx && py && ny && pz && nz)
				{
					m_Hint3dArray[x][y][z].markedForDeletion2 = true;
				}
			}
		}
	}
}

void qine::MergeHints()
{
	// Merging hints does not need to worry about texturing

	hintBrush currentHint;

	// merge x-axis
	for (int z = 0; z < m_Hint3dArray.size[0][0]() ; z++)
	{
		for (int y = 0; y < m_Hint3dArray.size[0]() ; y++)
		{
			for (int x = 0; x < m_Hint3dArray.size() ; x++)
			{
				if (x == 0)
				{
					currentHint = m_Hint3dArray[x][y][z];
				}
				else
				{
					if (!m_Hint3dArray[x][y][z].markedForDeletion &&
							!m_Hint3dArray[x][y][z].markedForDeletion2)
					{
						currentHint.width += m_HintSize;
						m_Hint3dArray[x][y][z].markedForDeletion = true;
					}
					else
					{
						currentHint = m_Hint3dArray[x][y][z];
					}
				}
			}
		}
	}

}

int qine::Optimize(int direction)
{
	int textureMask;

	int merged = 0;
	int added = 0;
	bool mergableBlock;

	vector<mapBlock> tempMapBlockCollection;
	vector<int> markedForDeletion;
	mapBlock currentMapBlock;

	switch(direction)
	{
	case optimizeByX:
		textureMask = 0x33;
		sort (m_BlockCollection.begin(), m_BlockCollection.end() , mapBlockComparatorX);
		break;
	case optimizeByY:
		textureMask = 0x0F;
		sort (m_BlockCollection.begin(), m_BlockCollection.end() , mapBlockComparatorY);
		break;
	case optimizeByZ:
		textureMask = 0x3C;
		sort (m_BlockCollection.begin(), m_BlockCollection.end() , mapBlockComparatorZ);
		break;
	}

	while (!m_BlockCollection.empty()) {

		// Take out the first block
		currentMapBlock = m_BlockCollection.front();
		m_BlockCollection.erase(m_BlockCollection.begin());
		added = 0;

		// Check all the objects for the one you want
		for (int i = 0; i < m_BlockCollection.size(); i++)
		{
			mergableBlock = false;

			// Block suitable for merge
			switch(direction)
				{
				case optimizeByX:
					if (
										(m_BlockCollection.at(i).blck.x == currentMapBlock.blck.x + 1 + added) &&
										(m_BlockCollection.at(i).blck.y == currentMapBlock.blck.y) &&
										(m_BlockCollection.at(i).blck.z == (currentMapBlock.blck.z)) &&
										(m_BlockCollection.at(i).blck.type == currentMapBlock.blck.type) &&
										((m_BlockCollection.at(i).texturing & 0x33) == (currentMapBlock.texturing & 0x33)) &&
										(m_BlockCollection.at(i).blck.height == currentMapBlock.blck.height) &&
										(m_BlockCollection.at(i).blck.length == currentMapBlock.blck.length))
								{
						currentMapBlock.blck.width++;
						mergableBlock = true;
								}
					break;
				case optimizeByY:
					if ( (m_BlockCollection.at(i).blck.x == currentMapBlock.blck.x) &&
										(m_BlockCollection.at(i).blck.y == currentMapBlock.blck.y + 1 + added) &&
										(m_BlockCollection.at(i).blck.z == (currentMapBlock.blck.z)) &&
										(m_BlockCollection.at(i).blck.type == currentMapBlock.blck.type) &&
										((m_BlockCollection.at(i).texturing & 0xF) == (currentMapBlock.texturing & 0xF)) &&
										(m_BlockCollection.at(i).blck.height == currentMapBlock.blck.height) &&
										(m_BlockCollection.at(i).blck.width == currentMapBlock.blck.width)
								) {
						currentMapBlock.blck.length++;
						mergableBlock = true;
					}

					break;
				case optimizeByZ:
					if ( (m_BlockCollection.at(i).blck.x == currentMapBlock.blck.x) &&
										(m_BlockCollection.at(i).blck.y == currentMapBlock.blck.y) &&
										(m_BlockCollection.at(i).blck.z == (currentMapBlock.blck.z -1 - added)) &&
										(m_BlockCollection.at(i).blck.type == currentMapBlock.blck.type) &&
										((m_BlockCollection.at(i).texturing & 0x3C) == (currentMapBlock.texturing & 0x3C)) &&
										(m_BlockCollection.at(i).blck.width == currentMapBlock.blck.width) &&
										(m_BlockCollection.at(i).blck.length == currentMapBlock.blck.length))
								{
						currentMapBlock.blck.height++;
						mergableBlock = true;
								}

					break;
				}

			if (mergableBlock)
			{
					currentMapBlock.texturing |= m_BlockCollection.at(i).texturing;
					added++;
					merged++;
					markedForDeletion.push_back(i);
			}
		}

		// delete the marked blocks
		while (!markedForDeletion.empty()) {
			m_BlockCollection.erase(m_BlockCollection.begin() + markedForDeletion.back());
			markedForDeletion.pop_back();
		}
		markedForDeletion.clear();
		tempMapBlockCollection.push_back(currentMapBlock);
	}

	m_BlockCollection = tempMapBlockCollection;
	return merged;
}

/*
 * Creates a map file from the created collection of blocks.
 */
void qine::createMapFile(std::string mapname)
{
	if (m_BlockCollection.size() == 0)
	{
		cout << "--- ERROR: There are no blocks in the collection" << endl;
		return;
	}

	cout << "Writing map file..." << endl;

	m_OutFile.open(mapname.c_str());
	m_OutFile << "{" << endl << "\"classname\" \"worldspawn\"" << endl;

	// TODO: Add options for custom blocksize and chopsize
	// m_OutFile << "\"_blocksize\" \"4096 4096 4096\"" << endl;
	// m_OutFile << "\"_blocksize\" \"32768 32768 32768\"" << endl;
	// m_OutFile << "\"chopsize\" \"4096\"" << endl;

	for (int i = 0; i <  m_BlockCollection.size(); i++)
	{
		createBrush( m_BlockCollection[i].blck.x,
				 m_BlockCollection[i].blck.y,
				 m_BlockCollection[i].blck.z,
				 m_BlockCollection[i].blck.width,
				 m_BlockCollection[i].blck.length,
				 m_BlockCollection[i].blck.height,
				 m_BlockCollection[i].blck.type,
				 m_BlockCollection[i].texturing);
	}

	cout << "brushes done" << endl;

	for (int z = 0; z < m_Hint3dArray[0][0].size(); z++)
	{
		for (int y = 0; y < m_Hint3dArray[0].size(); y++)
		{
			for (int x = 0; x < m_Hint3dArray.size(); x++)
			{
				if (!m_Hint3dArray[x][y][z].markedForDeletion
						&& !m_Hint3dArray[x][y][z].markedForDeletion2)
				{
				createBrush(
						m_Hint3dArray[x][y][z].x,
						m_Hint3dArray[x][y][z].y,
						m_Hint3dArray[x][y][z].z,
						m_Hint3dArray[x][y][z].width,
						m_Hint3dArray[x][y][z].length,
						m_Hint3dArray[x][y][z].height,
								0xFFFF,
								0xFF // all sides textured on hints
								);
				}
			}
		}
	}

	// TODO: Add all entities such as flowers.
	m_OutFile << "}" << endl;
	m_OutFile.close();
}

/*
 * Write one brush to the file.
 */
void qine::createBrush(int x, int y, int z, int length, int y_length, int height, int type, int texturing)
{
	int brushSize = 64;
	int blockflags = 0;

	string xp_tex;
	string xn_tex;
	string yp_tex;
	string yn_tex;
	string zp_tex;
	string zn_tex;

	getTextures(type, texturing, xp_tex, xn_tex, yp_tex, yn_tex, zp_tex, zn_tex, blockflags);

	if(type == Hint) blockflags = 0;

	m_OutFile << "{" << endl;

	char temp[512];
	// left -x
	sprintf(temp, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.25 0.25 %d 0 0\n",
			x*brushSize, 0,0,
			x*brushSize, 1,0,
			x*brushSize, 0,1,
			xn_tex.c_str(), blockflags);

	m_OutFile << temp;

	// right +x
	sprintf(temp, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.25 0.25 %d 0 0\n",
			(x+length)*brushSize, 0,0,
			(x+length)*brushSize, 0,1,
			(x+length)*brushSize, 1,0,
			xp_tex.c_str(), blockflags);

			m_OutFile << temp;

	// front y-
	sprintf(temp, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.25 0.25 %d 0 0\n",
			0,y*brushSize, 0,
			0,y*brushSize, 1,
			1,y*brushSize, 0,
			yn_tex.c_str(), blockflags);

			m_OutFile << temp;

	// back y+
	sprintf(temp, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.25 0.25 %d 0 0\n",
			0, (y+y_length)*brushSize, 0,
			1, (y+y_length)*brushSize, 0,
			0, (y+y_length)*brushSize, 1,
			yp_tex.c_str(), blockflags);

			m_OutFile << temp;

	// bottom z-
	sprintf(temp, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.25 0.25 %d 0 0\n",
			0, 0, (z-height)*brushSize,
			1, 0, (z-height)*brushSize,
			0, 1, (z-height)*brushSize,
			zn_tex.c_str(), blockflags);

			m_OutFile << temp;

	// top z+
	sprintf(temp, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.25 0.25 %d 0 0\n",
			0, 0, z*brushSize,
			0, 1, z*brushSize,
			1, 0, z*brushSize,
			zp_tex.c_str(), blockflags);

	m_OutFile << temp;

	m_OutFile << "}" << endl;
}

void qine::getTextures(int type, int texturing, string& xp_tex, string& xn_tex, string& yp_tex, string& yn_tex, string& zp_tex, string& zn_tex, int &blockflags)
{
	string top;
	string side;
	string bottom;
	blockflags = 0;
	string caulk;



	if (type == Water || type == StationaryWater) {
		caulk = "minetex/water_invis";
		blockflags = 134217728;
	} else {
		caulk = "common/caulk";
	}

	if (m_HintSize > 0)
	{
		blockflags = 134217728;
	}

	switch (type) {
	case Grass:
		top = "minetex/minetex-000"; // Grass
		side = "minetex/minetex-003"; // Grass dirt
		bottom = "minetex/minetex-002"; // Dirt
		break;

	case Stone:
		top = "minetex/minetex-001";
		side = "minetex/minetex-001";
		bottom = "minetex/minetex-001";
		break;

	case Dirt:
		top = "minetex/minetex-002";
		side = "minetex/minetex-002";
		bottom = "minetex/minetex-002";
		break;

	case StationaryWater:
	case Water:
		top = "liquids/clear_calm1";
		side = "liquids/clear_calm1";
		bottom = "liquids/clear_calm1";
		break;

	case Lava:
	case StationaryLava:
		top = "liquids/lavahell_2000";
		side = "liquids/lavahell_2000";
		bottom = "liquids/lavahell_2000";
		break;

	case Wood:
		blockflags = 134217728;
		top = "minetex/minetex-021";
		side = "minetex/minetex-020";
		bottom = "minetex/minetex-021";
		break;

	case Leaves:
		blockflags = 134217728;
		top = "minetex/minetex-052";
		side = "minetex/minetex-052";
		bottom = "minetex/minetex-052";
		break;

	case Bedrock:
		top = "minetex/minetex-016";
		side = "minetex/minetex-016";
		bottom = "minetex/minetex-016";
		break;

	case Gravel:
	case Cobblestone:
		top = "minetex/minetex-019";
		side = "minetex/minetex-019";
		bottom = "minetex/minetex-019";
		break;

	case Sand:
		top = "minetex/minetex-018";
		side = "minetex/minetex-018";
		bottom = "minetex/minetex-018"; // Sand
		break;

	case GoldOre:
		top = "minetex/minetex-032";
		side = "minetex/minetex-032";
		bottom = "minetex/minetex-032";
		break;

	case IronOre:
		top = "minetex/minetex-033";
		side = "minetex/minetex-033";
		bottom = "minetex/minetex-033";
		break;

	case CoalOre:
		top = "minetex/minetex-034";
		side = "minetex/minetex-034";
		bottom = "minetex/minetex-034";
		break;

	case Hint:
		top = "common/hint";
		side = "common/hint";
		bottom = "common/hint";
		break;

	default:
		top = "minetex/minetex-024";
		side = "minetex/minetex-024";
		bottom = "minetex/minetex-024";
		break;
	}

	  // caulk x+
	if ((texturing & xp) == 0)
		xp_tex = caulk;
	else
		xp_tex = side;

	// caulk x-
	if ((texturing & xm) == 0)
		xn_tex = caulk;
	else
		xn_tex = side;

	//  y+
	if ((texturing & yp) == 0)
		yp_tex = caulk;
	else
		yp_tex = side;

	// y-
	if ((texturing & ym) == 0)
		yn_tex = caulk;
	else
		yn_tex = side;

	// z+
	if ((texturing & zp) == 0) {
		zp_tex = caulk;
	}
	else {
		zp_tex = top;
	}

	 // z-
	if ((texturing & zm) == 0)
		zn_tex = caulk;
	else
		zn_tex = bottom;
}

void qine::printLayer(int a_z, int size) {
	char ch;
	cout << "---" << endl;
	for (int i_y = 0; i_y < size; i_y++) {
		for (int i_x = 0; i_x < size; i_x++) {
			ch = getBlockAtXYZ(m_Blocks, i_x, i_y, a_z);
			 cout << hex << setfill('0') << setw(2) << (int)ch << " ";
		}
		cout << endl;
	}
	cout << endl << endl;
}

int qine::blockCount()
{
	return m_BlockCollection.size();
}

} /* namespace qine */

