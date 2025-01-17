#include "NavigationGrid.h"
#include "../../Common/Assets.h"

#include <fstream>

using namespace NCL;
using namespace CSC8503;

const int LEFT_NODE = 0;
const int RIGHT_NODE = 1;
const int TOP_NODE = 2;
const int BOTTOM_NODE = 3;

const char WALL_NODE = 'x';
const char FLOOR_NODE = '.';
const char COST_4 = '#';
const char COST_3 = '*';
const char COST_5 = '-';


NavigationGrid::NavigationGrid() {
	nodeSize = 0;
	gridWidth = 0;
	gridHeight = 0;
	allNodes = nullptr;
}

NavigationGrid::NavigationGrid(const std::string& filename) : NavigationGrid() { //making grid maze file
	std::ifstream infile(Assets::DATADIR + filename);

	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;



	allNodes = new GridNode[gridWidth * gridHeight];

	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * y) + x];
			char type = 0;
			infile >> type;
			n.type = type;
			n.position = Vector3((float)(x * nodeSize - 95), 0, (float)(y * nodeSize - 95)); /////////////////////////////
		}
	}

	//now to build the connectivity between the nodes
	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * y) + x];

			if (y > 0) { //get the above node
				n.connected[0] = &allNodes[(gridWidth * (y - 1)) + x];
			}
			if (y < gridHeight - 1) { //get the below node
				n.connected[1] = &allNodes[(gridWidth * (y + 1)) + x];
			}
			if (x > 0) { //get left node
				n.connected[2] = &allNodes[(gridWidth * (y)) + (x - 1)];
			}
			if (x < gridWidth - 1) { //get right node
				n.connected[3] = &allNodes[(gridWidth * (y)) + (x + 1)];
			}
			for (int i = 0; i < 4; ++i) { /////////////////////////////////////////////////////////////////
				if (n.connected[i]) {
					if (n.connected[i]->type == '.') {
						n.costs[i] = 1;
					}
					if (n.connected[i]->type == '#') {
						n.costs[i] = 4;
					}
					if (n.connected[i]->type == '*') {
						n.costs[i] = 3;
					}
					if (n.connected[i]->type == '-') {
						n.costs[i] = 5;
					}
					if (n.connected[i]->type == 'x') {
						n.connected[i] = nullptr; //actually a wall, disconnect!
					}
				}
			}
		}
	}
}

NavigationGrid::~NavigationGrid() {
	delete[] allNodes;
}


bool NavigationGrid::FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) {
	//need to work out which node 'from' sits in, and 'to' sits in
	int fromX = ((int)from.x / nodeSize);
	int fromZ = ((int)from.z / nodeSize);

	int toX = ((int)to.x / nodeSize);
	int toZ = ((int)to.z / nodeSize);

	if (fromX < 0 || fromX > gridWidth - 1 ||
		fromZ < 0 || fromZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	if (toX < 0 || toX > gridWidth - 1 ||
		toZ < 0 || toZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	//turning co-ordinates to index value
	GridNode* startNode = &allNodes[(fromZ * gridWidth) + fromX];
	GridNode* endNode = &allNodes[(toZ * gridWidth) + toX];

	std::vector<GridNode*>  openList;
	std::vector<GridNode*>  closedList;

	openList.emplace_back(startNode);

	//initialise starting node
	startNode->f = 0;
	startNode->g = 0;
	startNode->parent = nullptr;

	GridNode* currentBestNode = nullptr;

	while (!openList.empty()) {
		currentBestNode = RemoveBestNode(openList);

		if (currentBestNode == endNode) { //we've found the path to destination!
			GridNode* node = endNode;
			while (node != nullptr) {
				outPath.PushWaypoint(node->position); //build up the waypoints for ai
				node = node->parent;
			}
			return true;
		}
		else {
			for (int i = 0; i < 4; ++i) {
				GridNode* neighbour = currentBestNode->connected[i];

				if (!neighbour) { //might not be connected...
					continue;
				}
				bool inClosed = NodeInList(neighbour, closedList); //is neighbour in closed list already?
				if (inClosed) {
					continue; //already discarded this neighbour...
				}

				float h = Heuristic(neighbour, endNode);
				float g = currentBestNode->g + currentBestNode->costs[i]; //costs[i] is cost of traversing node, eg terrain
				float f = h + g;

				bool inOpen = NodeInList(neighbour, openList); //is neighbour in open list already?

				if (!inOpen) { //first time we've seen this neighbour
					openList.emplace_back(neighbour);
				}
				if (!inOpen || f < neighbour->f) {//might be a better route to this neighbour
					neighbour->parent = currentBestNode;
					neighbour->f = f;
					neighbour->g = g;
				}
			}
			closedList.emplace_back(currentBestNode); //place node into closed list
		}
	}
	return false; //open list emptied out with no path!
}


//return whether gridnode is in the list or not
bool NavigationGrid::NodeInList(GridNode* n, std::vector<GridNode*>& list) const {
	std::vector<GridNode*>::iterator i = std::find(list.begin(), list.end(), n);
	return i == list.end() ? false : true;
}

GridNode* NavigationGrid::RemoveBestNode(std::vector<GridNode*>& list) const {
	std::vector<GridNode*>::iterator bestI = list.begin();

	GridNode* bestNode = *list.begin();

	for (auto i = list.begin(); i != list.end(); ++i) {
		if ((*i)->f < bestNode->f) {
			bestNode = (*i);
			bestI = i; //always the best node
		}
	}
	list.erase(bestI);

	return bestNode;
}


//euclidean distance
float NavigationGrid::Heuristic(GridNode* hNode, GridNode* endNode) const {
	return (hNode->position - endNode->position).Length();
}