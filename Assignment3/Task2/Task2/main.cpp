/* 
* 
*/

#include <vector>
#include <iostream>
#include <fstream>
#include "tinyxml2.h"
#include <string>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>

using namespace std;

// direction of word in crossword
enum WordDirection { Across, Down };

// Crossword element Requirements
struct CrosswordElementReq {
	// requirements
	WordDirection dir;		// direction of words in crossword
	int size = 0;			// size of word (length)
	unsigned int index[2];	// location of word (row, col)
	int wordId;				// word identifier

	void operator=(const CrosswordElementReq& copy) {
		this->dir = copy.dir;
		this->index[0] = copy.index[0];
		this->index[1] = copy.index[1];
		this->size = copy.size;
		this->wordId = copy.wordId;
	}
};

// intersection indexing
struct Intersect {
	Intersect(int wordID, int wordIndex, int originWordindex)
		: wordId(wordID), wordIndex(wordIndex), originWordIndex(originWordindex) {}
	int wordId;				// word id 
	int wordIndex;
	int originWordIndex;
};
// wordId, intersection
unordered_map<int, vector<Intersect>> IntersectionMap;

// set of possible words that can be choosen for specified crossword element
struct CrosswordElementSet {
	int wordId;
	std::vector<uint16_t> words;
};

// Crossword Element (word and element id)
struct CrosswordElement {
	uint16_t word;
	int wordId;
};

// Loading from XML parameter file
vector<CrosswordElementReq> LoadWordRestrictions(const char* filename)
{
	vector<CrosswordElementReq> wordSet;
	std::cout << "Current Working Directory =" << filename << std::endl;

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(filename);

	if (result != tinyxml2::XML_SUCCESS) {
		printf("ERROR: Invalid Parameter File\a\n");
		exit(0);
	}

	tinyxml2::XMLElement* pRoot = doc.RootElement();

	for (tinyxml2::XMLElement* pElem = pRoot->FirstChildElement(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string elementName = pElem->Value();

		if (elementName == "CrosswordElement") {
			CrosswordElementReq temp;
			/*const char* stateName;
			pElem->QueryStringAttribute("name", &stateName);*/

			pElem->QueryAttribute("elementId", &temp.wordId);
			pElem->QueryAttribute("row", &temp.index[0]);
			pElem->QueryAttribute("col", &temp.index[1]);


			const char * direction;
			pElem->QueryStringAttribute(std::string("direction").c_str(), &direction);

			int size;
			pElem->QueryAttribute("size", &size);

			if (strcmp(direction,"V") == 0) {
				temp.size = size;
				temp.dir = WordDirection::Down;
			}
			else {
				temp.size = size;
				temp.dir = WordDirection::Across;
			}

			wordSet.push_back(temp);
		}
	}

	return wordSet;
}

void ItersectionFinder(vector<CrosswordElementReq>& wordSet) {
	for (auto& w1 : wordSet) {
		for (auto& w2 : wordSet) {
			if (w1.wordId == w2.wordId)
				continue;

			unsigned int endIndex1[2], endIndex2[2];
			if (w1.dir == WordDirection::Across) {
				endIndex1[0] = w1.index[0];
				endIndex1[1] = w1.index[1] + w1.size - 1;
			}
			else {
				endIndex1[1] = w1.index[1];
				endIndex1[0] = w1.index[0] + w1.size - 1;
			}
			if (w2.dir == WordDirection::Across) {
				endIndex2[0] = w2.index[0];
				endIndex2[1] = w2.index[1] + w2.size - 1;
			}
			else {
				endIndex2[1] = w2.index[1];
				endIndex2[0] = w2.index[0] + w2.size - 1;
			}


			int A1 = (endIndex1[1] - w1.index[1]);
			int A2 = (endIndex2[1] - w2.index[1]);
			int B1 = (w1.index[0] - endIndex1[0]);
			int B2 = (w2.index[0] - endIndex2[0]);

			// calculating the determinet to determine parallelism:
			// A = y2 - y1, B = x1 - x2
			int det = A1 * B2 - A2 * B1;

			// if det does't equal zero, then not parallel (intersection)
			if (det != 0) {
				// C = A*x1 + B*y
				int C1 = A1 * w1.index[0] + B1 * w1.index[1];
				int C2 = A2 * w2.index[0] + B2 * w2.index[1];

				// intersection point
				int row = ((float)(B2 * C1 - B1 * C2)) / ((float)det);
				int col = ((float)(A1 * C2 - A2 * C1)) / ((float)det);

				// verifying intersection is within domain
				if (w1.index[0] <= row && row <= endIndex1[0] && w2.index[0] <= row && row <= endIndex2[0]
					&& w1.index[1] <= col && col <= endIndex1[1] && w2.index[1] <= col && col <= endIndex2[1]) {
					// Finding String index points to compare from
					int wordIndex1, wordIndex2;

					// first word's index point
					if (w1.dir == WordDirection::Across) {
						wordIndex1 = col - w1.index[1];
					}
					else {
						wordIndex1 = row - w1.index[0];
					}

					// second word's index point
					if (w2.dir == WordDirection::Across) {
						wordIndex2 = col - w2.index[1];
					}
					else {
						wordIndex2 = row - w2.index[0];
					}

					// mapping intersection
					IntersectionMap[w1.wordId].push_back(Intersect(w2.wordId, wordIndex2, wordIndex1));
				}
			}
		}
	}
}

string Dictionary[21120];
// Data filter based on word restrictions from XML
vector<CrosswordElementSet> DictionaryFilter(vector<CrosswordElementReq>& wordSet) {
	// Getting all words from directory
	ifstream DictionaryFile("Dictionary.txt");
	string tmp; int i = 0;
	while (DictionaryFile >> tmp) { Dictionary[i] = tmp; i++; }

	vector<CrosswordElementSet> wordStates(wordSet.size());

	for (uint16_t i = 0; i < wordStates.size(); i++) {
		wordStates[i].wordId = wordSet[i].wordId;
		for (int j = 0; j < 21120; j++) {
			if (Dictionary[j].length() == wordSet[i].size)
				wordStates[i].words.push_back(j);
		}
	}
	return wordStates;
}

class DynamicArray {
public:
	DynamicArray(int numElements) {
		_size = 0;
		arr = new CrosswordElementSet[numElements];
	}
	void push_back(CrosswordElementSet wes) {
		arr[_size] = wes;
		_size++;
	}

	unsigned int size() { return _size; }

	CrosswordElementSet& operator[](int index) {
		return arr[index];
	}
	~DynamicArray() { delete[] arr; }
private:
	CrosswordElementSet* arr;
	unsigned int _size;
};

bool SelectNextElementSet(const CrosswordElement& newWord, vector<CrosswordElementSet>& wordSet, CrosswordElementSet& nextWords) {
	DynamicArray tmpSet(IntersectionMap[newWord.wordId].size());

	// finding intersecting word elements then determining if element set contains word that conforms with restrictions
	for (auto& sect : IntersectionMap[newWord.wordId]) {
		// determining if intersect exists
		auto it = find_if(wordSet.begin(), wordSet.end(), [&sect](const CrosswordElementSet& obj) { return obj.wordId == sect.wordId; });
		if (it != wordSet.end()) {
			CrosswordElementSet newState;
			newState.wordId = it->wordId;
			for (auto& word : it->words) {
				// checking intersection point between new word and this word
				if (Dictionary[word][sect.wordIndex] == Dictionary[newWord.word][sect.originWordIndex])
					newState.words.push_back(word);
			}
			// determining if anywords conformed to restrictions
			if (newState.words.size() > 0) {
				tmpSet.push_back(newState);
			}
			else // no word conformed
				return false;
		}
	}



	// if all intersections are in crossword
	if (tmpSet.size() == 0) {
		// bubble sort for number of solutions
		bool swapped;
		for (int i = wordSet.size() - 1; i >= 0; i--)
		{
			swapped = false;
			for (int j = wordSet.size() - 1; j > wordSet.size() - i - 1; j--)
			{
				if (wordSet[j].words.size() > wordSet[j - 1].words.size())
				{
					CrosswordElementSet tmp = wordSet[j];
					wordSet[j] = wordSet[j - 1];
					wordSet[j - 1] = tmp;
					swapped = true;
				}
			}

			// IF no two elements were swapped by inner loop, then break 
			if (swapped == false)
				break;
		}


		// get the last element and set it to next word
		nextWords = wordSet.back();
		wordSet.pop_back();			// remove last element
		return true;
	}

	// bubble sort for number of solutions
	bool swapped;
	for (int i = 0; i < tmpSet.size() - 1; i++)
	{
		swapped = false;
		for (int j = 0; j < tmpSet.size() - i - 1; j++)
		{
			if (tmpSet[j].words.size() > tmpSet[j + 1].words.size())
			{
				CrosswordElementSet tmp = tmpSet[j];
				tmpSet[j] = tmpSet[j + 1];
				tmpSet[j + 1] = tmp;
				swapped = true;
			}
		}

		// IF no two elements were swapped by inner loop, then break 
		if (swapped == false)
			break;
	}

	// set next element and remove same element from set
	nextWords = tmpSet[0];
	wordSet.erase(find_if(wordSet.begin(), wordSet.end(), [&tmpSet](const CrosswordElementSet& obj) { return obj.wordId == tmpSet[0].wordId; }));

	// replacing old words with new words (filtered words)
	for (int i = 1; i < tmpSet.size(); i++) {
		auto it = find_if(wordSet.begin(), wordSet.end(), [&tmpSet, &i](const CrosswordElementSet& obj) { return obj.wordId == tmpSet[i].wordId; });
		(*it) = tmpSet[i];
	}

	return true;
}

vector<CrosswordElement> Solution;
bool Backtracking(vector<CrosswordElement>& currWords, vector<CrosswordElementSet> wordSet) {
	CrosswordElementSet nextWords;
	if (SelectNextElementSet(currWords.back(), wordSet, nextWords)) {
		CrosswordElement newWord;
		newWord.wordId = nextWords.wordId;
		if (wordSet.size() == 0) {
			newWord.word = nextWords.words[0];
			currWords.push_back(newWord);
			Solution = currWords;
			return true;
		}
		for (auto& word : nextWords.words) {
			newWord.word = word;
			currWords.push_back(newWord);
			if (Backtracking(currWords, wordSet))
				return true;
			currWords.pop_back();
		}
	}
	return false;
}

int main() {
	// parsing words from XML file
	vector<CrosswordElementReq> crosswordElementReq = LoadWordRestrictions("heartCrossword.xml");

	// populating intersection map
	ItersectionFinder(crosswordElementReq);

	// getting words
	vector<CrosswordElementSet> wordSet = DictionaryFilter(crosswordElementReq);

	// sorting by number of possible solutions (number of words)
	bool swapped;
	for (int i = wordSet.size() - 1; i >= 0; i--)
	{
		swapped = false;
		for (int j = wordSet.size() - 1; j > wordSet.size() - i - 1; j--)
		{
			if (wordSet[j].words.size() > wordSet[j - 1].words.size())
			{
				CrosswordElementSet tmp = wordSet[j];
				wordSet[j] = wordSet[j - 1];
				wordSet[j - 1] = tmp;
				swapped = true;
			}
		}

		// IF no two elements were swapped by inner loop, then break 
		if (swapped == false)
			break;
	}

	CrosswordElementSet startingWords = wordSet.back();
	wordSet.pop_back();

	vector<CrosswordElement> set;
	set.reserve(crosswordElementReq.size());
	CrosswordElement initWord; initWord.wordId = startingWords.wordId;
	set.push_back(initWord);

	// starting timer
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	for (auto& word : startingWords.words) {
		set[0].word = word;
		if (Backtracking(set, wordSet))
			break;
	}
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	if (Solution.size() > 0) {
		// sorting by id
		bool swapped;
		for (int i = 0; i < Solution.size() - 1; i++)
		{
			swapped = false;
			for (int j = 0; j < Solution.size() - i - 1; j++)
			{
				if (Solution[j].wordId > Solution[j + 1].wordId)
				{
					CrosswordElement tmp = Solution[j];
					Solution[j] = Solution[j + 1];
					Solution[j + 1] = tmp;
					swapped = true;
				}
			}

			// IF no two elements were swapped by inner loop, then break 
			if (swapped == false)
				break;
		}

		printf("CrosswordElement ID | Word\n");
		printf("========================\n");
		for (auto& word : Solution) {
			printf("%-4i | %s\n", word.wordId, Dictionary[word.word].c_str());
		}

		printf("\n\t *Negative IDs are the horizontal (across) counterpart for vertical/horizontal words\n");
	}
	else {
		printf("NO SOLUTION\n\a");
	}
	fflush(stdout);


	std::cout << "\nExecution Time = " << ((double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) * 1e-6 << "[s]" << std::endl;

	return 0;
}