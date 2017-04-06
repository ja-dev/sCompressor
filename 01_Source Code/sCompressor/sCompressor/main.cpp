/* On my honor, I have neither given nor received
unauthorized aid on this assignment */

#include<iostream>
#include <vector>
#include <deque>
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <limits>   
#include <numeric> 

using namespace std;

class compressUtil
{

private:
	//map<string, int> dictionary;
	vector<pair<string, int>> dictionary;
	map<int, string, greater<int>> orderedDictionary;


	vector<string> program;
	vector<string> compressedProgram;

	string cSerializedProgram;
	string decompressedProgram;

	enum COMPRESSTION_TYPE
	{
		ORIG, 
		RLE, 
		BITMASK, 
		ONE_BIT_M, 
		TWO_BIT_CM, 
		FOUR_BIT_CM, 
		TWO_BIT_AM, 
		DIRECT
	} cTypes;

	class HEADERS 
	{
	public:
		const string ORIGINA_BIN ="000";
		const string RLE = "001";
		const string BITMASK = "010";
		const string ONE_BIT_M = "011";
		const string TWO_BIT_CM = "100";
		const string FOUR_BIT_CM = "101";
		const string TWO_BIT_AM = "110";
		const string DIRECT = "111";
	}C_HEADERS;

	class COMPRESSION_WIDTHS
	{
	public:
		static const int ORIGINA_BIN = 35;
		static const int RLE = 6;
		static const int BITMASK = 16;
		static const int ONE_BIT_M = 12;
		static const int TWO_BIT_CM = 12;
		static const int FOUR_BIT_CM = 12;
		static const int TWO_BIT_AM = 17;
		static const int DIRECT = 7;
	};

	

	int getwidthDifference(const string& s1, const string& s2, pair<int,int>& range, const int instructionWidth = 32)
	{
		int l{}, r{};

		auto itr1 = s1.cbegin();
		auto itr2 = s2.cbegin();

		while (itr1!=s1.cend())
		{
			if (*itr1 == *itr2)
				l++;
			else
				break;

			itr1++; itr2++;
		}

		auto ritr1 = s1.crbegin();
		auto ritr2 = s2.crbegin();

		while (ritr1 != s1.crend())
		{
			if (*ritr1 == *ritr2)
				r++;
			else
				break;

			ritr1++; ritr2++;
		}

		range.first = l;
		range.second = (32 - r) - 1; //Check if universal

		return abs(range.first - range.second);

	}

	char singleCharXor(char c1, char c2)
	{
		if ((c1 == '0') && (c2 == '0'))
			return char('0');

		if (((c1 == '1') && (c2 == '0'))|| (c1 == '0') && (c2 == '1'))
			return char('1');

		if ((c1 == '1') && (c2 == '1'))
			return char('0');
	}

	char singleCharFlip(char c1)
	{
		if (c1 == '0') 
			return char('1');  
		else 
			return char('0');
	}

	int getCharacterDifference(const string& s1, const string& s2) const
	{
		return inner_product(
			s1.begin(), s1.end(), s2.begin(),
			0, std::plus<unsigned int>(),
			std::not2(std::equal_to<std::string::value_type>())
		);
	}

	string binary(unsigned x)
	{
		string s;
		do
		{
			s.push_back('0' + (x & 1));
		} while (x >>= 1);
		std::reverse(s.begin(), s.end());
		return s;
	}

	string leftPad(string s1, int padLength)
	{
		s1.insert(s1.begin(), padLength - s1.size(), '0');
		return s1;
	}

	void retrieveDictionary(string pInput)
	{
		fstream infile(pInput);
		string tLine;
		bool capture = false;
		int c{};

		while (getline(infile, tLine))
		{
			if (tLine == "xxxx") 
			{
				capture = true; 
				continue;
			}
			if (!capture) 
			{
				cSerializedProgram = cSerializedProgram + tLine;
				continue;
			}
			dictionary.push_back(make_pair(tLine, c++));
		}
	}

	void prepareDictionary(string pInput)
	{
		fstream infile(pInput);
		string tLine;
		int order{};
		unordered_map<string, pair<int, int>> tStorage;

		program.push_back(string());

		while (getline(infile, tLine))
		{
			program.push_back(tLine);
			auto potentialEntry = tStorage.find(tLine);

			if (potentialEntry != tStorage.end())
				potentialEntry->second.first++;
			else
				tStorage.insert(make_pair(tLine, make_pair(1, order)));

			order++;
		}

		multimap<int, pair<string, int>, greater<int>> tDictionary;
		vector<string> printReadyDictionary;

		for (auto& potentialEntry : tStorage)
		{
				tDictionary.insert(make_pair(potentialEntry.second.first,
					make_pair(potentialEntry.first, potentialEntry.second.second)));
		}

		int numberOfEntries{};

		while(true)
		{
			//itr (tDictionary) format: Frequency -> instruction, order
			auto equalFRange = tDictionary.equal_range(tDictionary.begin()->first);

			map<int, pair<string, int>> tContain;
			//order -> instruction, frequency

			for (auto itr = equalFRange.first; itr != equalFRange.second; itr++)
				tContain.insert(make_pair(itr->second.second, make_pair(itr->second.first, itr->first)));

			for (const auto& entry : tContain)
			{
				printReadyDictionary.push_back(entry.second.first);
				numberOfEntries++;
				
				//If 16 entries generated, make dictionary and return
				if (numberOfEntries == 16)
				{
					int c{};

					for (auto& entry : printReadyDictionary)
						dictionary.push_back(make_pair(entry, c++));

					return;
				}
			}

			tContain.clear();
			tDictionary.erase(tDictionary.begin()->first);

		}

	}

	string getCompressedInstruction(string instruction, int& lineNumber)
	{

		multimap<int, pair<string, int>, less<int>> compresssedInstructionCollection;
		//Multimap format: size of instruction, compressed instruction, instruction increment count

		string tCompressedInstruction{};
		int instructionSize{};
		int instructionIncrement{};

		//Generate set of all possible compressions for given instruction
		//####################################################################################################
		for(const auto& compressionMethod: {0,1,2,3,4,5,6,7})
		{
			tCompressedInstruction = compressInstructionUsing(instruction, compressionMethod, lineNumber, instructionIncrement);
			instructionSize = tCompressedInstruction == "" ? numeric_limits<int>::max() : tCompressedInstruction.length();

			auto ciEntry = make_pair(instructionSize, make_pair(tCompressedInstruction, instructionIncrement));
			compresssedInstructionCollection.insert(ciEntry);

			instructionIncrement = 0;
		}

		//Choose most prioritized from equally sized compressed instructions
		//####################################################################################################

		//RLE almost gets the highest priority no matter what
		for (const auto& entry : compresssedInstructionCollection)
		{
			if (entry.second.first.find('-') != string::npos)
			{
				lineNumber += entry.second.second;
				return entry.second.first;
			}
		}

		/*if (compresssedInstructionCollection.find(6) != compresssedInstructionCollection.end())
		{
			lineNumber += compresssedInstructionCollection.find(6)->second.second;
			return compresssedInstructionCollection.find(6)->second.first;
		}
		*/
		auto equallyCompressedRange = compresssedInstructionCollection.equal_range(compresssedInstructionCollection.begin()->first);
		
		if ((distance(equallyCompressedRange.first,equallyCompressedRange.second)==1))
		{
			//When there's only one similarly sized compressed instruction
			lineNumber += compresssedInstructionCollection.begin()->second.second;
			return compresssedInstructionCollection.begin()->second.first;
		}
		else
		{
			//When there're many compressed instructions of the same size
			string code{};
			code += equallyCompressedRange.first->second.first[0];
			code += equallyCompressedRange.first->second.first[1];
			code += equallyCompressedRange.first->second.first[2];

			int priorityMethod = stoi(code);

			for (const auto& compressedInstruction : compresssedInstructionCollection)
			{
				string tCode{};
				tCode += compressedInstruction.second.first[0];
				tCode += compressedInstruction.second.first[1];
				tCode += compressedInstruction.second.first[2];

				if (priorityMethod > stoi(tCode)) priorityMethod = stoi(tCode);
			}

			for (const auto& compressedInstruction : compresssedInstructionCollection)
			{
				string tCode{};
				tCode += compressedInstruction.second.first[0];
				tCode += compressedInstruction.second.first[1];
				tCode += compressedInstruction.second.first[2];

				if (priorityMethod == stoi(tCode))
				{
					lineNumber += compressedInstruction.second.second;
					return compressedInstruction.second.first;
				}

			}

		}
		//####################################################################################################

	
	}

	string compressInstructionUsing(string instruction, int compressionMethod, int& lineNumber, int& instructionIncrement)
	{
		COMPRESSTION_TYPE currentCType = (COMPRESSTION_TYPE)compressionMethod;

		switch (currentCType)
		{
			case COMPRESSTION_TYPE::ORIG:
			{
				instructionIncrement = 1;
				return (C_HEADERS.ORIGINA_BIN + instruction);
				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::RLE:
			{
				if (lineNumber == program.size() - 1) return string();
				if (program[lineNumber] != program[lineNumber + 1]) return string();
				int occurences{};
				int lineCounter = lineNumber;

				while ((lineCounter != program.size() - 1) && (program[lineCounter] == program[lineCounter + 1]))
				{
					occurences++; 
					lineCounter++;
					if (occurences == 8) break;
				}occurences++;//Verify this increment
				
				//Find the best way to compress the first occurence
				string tCompressedInstruction{};
				map<int, string, less<int>> compresssedHeaderInstructionCollection;
				int instructionRLEheaderCount;

				for (const auto& compressionMethod : { 0,2,3,4,5,6,7 })
				{
					int tempLineNumber = lineNumber;
					int instructionSize{};

					tCompressedInstruction = compressInstructionUsing(instruction, compressionMethod, tempLineNumber, instructionRLEheaderCount);
					instructionSize = tCompressedInstruction == "" ? numeric_limits<int>::max() : tCompressedInstruction.length();

					auto ciEntry = make_pair(instructionSize, tCompressedInstruction);
					compresssedHeaderInstructionCollection.insert(ciEntry);
				}
				
				//Choose most prioritized from equally sized compressed header instructions
				//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
				auto equallyCompressedRange = compresssedHeaderInstructionCollection.equal_range(compresssedHeaderInstructionCollection.begin()->first);

				if (distance(equallyCompressedRange.first,equallyCompressedRange.second)==1)
				{
					//When there's only one similarly sized compressed instruction
					tCompressedInstruction= compresssedHeaderInstructionCollection.begin()->second;
				}
				else
				{
					//When there're many compressed instructions of the same size
					string code{};
					code += equallyCompressedRange.first->second[0];
					code += equallyCompressedRange.first->second[1];
					code += equallyCompressedRange.first->second[2];

					int priorityMethod = stoi(code);

					for (const auto& compressedInstruction : compresssedHeaderInstructionCollection)
					{
						string tCode{};
						tCode += compressedInstruction.second[0];
						tCode += compressedInstruction.second[1];
						tCode += compressedInstruction.second[2];

						if (priorityMethod > stoi(tCode)) priorityMethod = stoi(tCode);
					}

					for (const auto& compressedInstruction : compresssedHeaderInstructionCollection)
					{
						string tCode{};
						tCode += compressedInstruction.second[0];
						tCode += compressedInstruction.second[1];
						tCode += compressedInstruction.second[2];

						if (priorityMethod == stoi(tCode))
						{
							tCompressedInstruction = compressedInstruction.second; 
							break;
						}
					}

				}
				//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

				instructionIncrement += (occurences);// +instructionRLEheaderCount);

				return (tCompressedInstruction + "-" + C_HEADERS.RLE + leftPad(binary(occurences-2),3));

				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::BITMASK:
			{
				string mask = "0000";
				string sLocation;
				string	dictionaryIndex;
				bool flag = false;
				int charDiffernece{};
				int widthDifference{};

				for (auto dInstruction : dictionary)
				{
					if (dInstruction.first == instruction) return string();

					pair<int, int> diffRange;

					charDiffernece= getCharacterDifference(instruction, dInstruction.first);
					widthDifference = getwidthDifference(instruction, dInstruction.first, diffRange);

					
					if (((charDiffernece == 2) && //Extreme ended difference DxDD or DDxD
						(widthDifference > 1) &&(widthDifference < 4)) ||
						((charDiffernece==3)) &&  //Varied 3 bit differences
						(widthDifference ==2)|| widthDifference == 3)
					{
						mask[0] = (instruction[diffRange.first] == dInstruction.first[diffRange.first]) ? '0' : '1';
						mask[1] = (instruction[diffRange.first+1] == dInstruction.first[diffRange.first+1]) ? '0' : '1';
						mask[2] = (instruction[diffRange.first+2] == dInstruction.first[diffRange.first+2]) ? '0' : '1';
						mask[3] = (instruction[diffRange.first+3] == dInstruction.first[diffRange.first+3]) ? '0' : '1';

						sLocation = leftPad(binary(diffRange.first), 5);
						dictionaryIndex = leftPad(binary(dInstruction.second), 4);
						flag = true;
						break;
					}

				}
				if (flag == false) return string();

				instructionIncrement = 1;

				return (C_HEADERS.BITMASK + sLocation + leftPad(mask,4) + dictionaryIndex);
				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::ONE_BIT_M:
			{
				pair<int, int> sLocation;
				int dictionaryIndex{};
				bool flag = false;

				for (auto dInstruction : dictionary)
				{
					if (dInstruction.first == instruction) return string();
					if (getCharacterDifference(instruction, dInstruction.first) == 1)
					{
						getwidthDifference(instruction, dInstruction.first, sLocation);
						dictionaryIndex = dInstruction.second;
						flag = true;
						break;
					}
				}
				
				if (flag == false) return string();
				
				instructionIncrement = 1;

				return (C_HEADERS.ONE_BIT_M + leftPad(binary(sLocation.first),5) + leftPad(binary(dictionaryIndex),4));
				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::TWO_BIT_CM:
			{
				pair<int, int> sLocation;
				int dictionaryIndex{};
				bool flag = false;

				for (auto dInstruction : dictionary)
				{
					if (dInstruction.first == instruction) return string();

					if (getCharacterDifference(instruction, dInstruction.first) == 2)
					{
						if(getwidthDifference(instruction, dInstruction.first, sLocation)!=1) break;
						dictionaryIndex = dInstruction.second;
						flag = true;
						break;
					}
				}

				if (flag == false) return string();

				instructionIncrement = 1;

				return (C_HEADERS.TWO_BIT_CM + leftPad(binary(sLocation.first), 5) + leftPad(binary(dictionaryIndex), 4));
				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::FOUR_BIT_CM:
			{
				pair<int, int> sLocation;
				int dictionaryIndex{};
				bool flag = false;

				for (auto dInstruction : dictionary)
				{
					if (dInstruction.first == instruction) return string();

					if (getCharacterDifference(instruction, dInstruction.first) == 4)
					{
						if(getwidthDifference(instruction, dInstruction.first, sLocation)!=3) break;
						dictionaryIndex = dInstruction.second;
						flag = true;
						break;
					}
				}

				if (flag == false) return string();

				instructionIncrement = 1;

				return (C_HEADERS.FOUR_BIT_CM + leftPad(binary(sLocation.first),5) + leftPad(binary(dictionaryIndex),4));
				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::TWO_BIT_AM:
			{
				pair<int, int> sLocation;
				int dictionaryIndex{};
				bool flag = false;

				for (auto dInstruction : dictionary)
				{
					if (dInstruction.first == instruction) return string();

					if (getCharacterDifference(instruction, dInstruction.first) == 2)
					{
						getwidthDifference(instruction, dInstruction.first, sLocation);
						dictionaryIndex = dInstruction.second;
						flag = true;
						break;
					}
				}

				if (flag == false) return string();

				instructionIncrement = 1;

				return (C_HEADERS.TWO_BIT_AM + leftPad(binary(sLocation.first),5) + leftPad(binary(sLocation.second),5) + leftPad(binary(dictionaryIndex),4));
				break;
			}

			//####################################################################################################
			case COMPRESSTION_TYPE::DIRECT:
			{
				instructionIncrement = 1;

				for (auto dInstruction : dictionary)
					if (dInstruction.first == instruction)
						return (C_HEADERS.DIRECT + leftPad(binary(dInstruction.second),4));

				return string();
				break;
			}
		}
	}

	void compressProgam()
	{
		for (int itr = 1; itr < program.size();)
			compressedProgram.push_back(getCompressedInstruction(program[itr], itr));
	}

	void decompressFile()
	{
		int cCode{};
		int lineNumber{};

		string previousInstruction{};
		string currentInstruction{};
		string dictionaryEntry{};
		int dictionaryIndex{};
		cCode = cSerializedProgram.length();

		for (int index = 0; index<cSerializedProgram.length();)
		{
			cCode = stoi(cSerializedProgram.substr(index, 3),nullptr,2);
			index += 3;

			switch (cCode)
			{
				//####################################################################################################
				case (COMPRESSTION_TYPE::ORIG):
				{
					int trail = cSerializedProgram.length()-index;

					if ((trail)<=32)
					if ((stoi(cSerializedProgram.substr(index)) == 0)) return;

					currentInstruction
						= previousInstruction 
						= cSerializedProgram.substr(index, 32);

					decompressedProgram += currentInstruction; 

					index += 32;

					lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::RLE):
				{
					int itr{};
					int repetitions 
						= itr 
						= stoi(cSerializedProgram.substr(index, 3), nullptr, 2) + 1; //+1 to handle the base offset

					while (itr)
					{
						decompressedProgram += previousInstruction; 
						lineNumber++;
						itr--;
					}

					previousInstruction = "";
					index += 3;

					//lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::BITMASK):
				{
					int sLocation{};
					string decompressedInstruction{};

					string bitmask{};

					sLocation=stoi(cSerializedProgram.substr(index, 5), nullptr, 2); 
					//Starting Location captured
					index += 5;

					bitmask += cSerializedProgram[index];
					bitmask += cSerializedProgram[index + 1];
					bitmask += cSerializedProgram[index + 2];
					bitmask += cSerializedProgram[index + 3];
					//Mask captured
					index += 4;

					dictionaryIndex = stoi(cSerializedProgram.substr(index, 4), nullptr, 2);
					//Dictionary Index captured
					index += 4;

					dictionaryEntry 
						= currentInstruction
						= dictionary[dictionaryIndex].first;

					currentInstruction[sLocation] =
						singleCharXor(bitmask[0],dictionaryEntry[sLocation]);

					currentInstruction[sLocation+1] =
						singleCharXor(bitmask[1], dictionaryEntry[sLocation+1]);

					currentInstruction[sLocation+2] =
						singleCharXor(bitmask[2], dictionaryEntry[sLocation+2]);

					currentInstruction[sLocation+3] =
						singleCharXor(bitmask[3], dictionaryEntry[sLocation+3]);

					previousInstruction = currentInstruction;

					decompressedProgram += currentInstruction; 

					lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::ONE_BIT_M):
				{
					int mismatchLocation{};

					mismatchLocation = stoi(cSerializedProgram.substr(index, 5), nullptr, 2);
					//Mismatch Location captured
					index += 5;

					dictionaryIndex = stoi(cSerializedProgram.substr(index, 4), nullptr, 2);
					//Dictionary Index captured
					index += 4;

					dictionaryEntry
						= currentInstruction
						= dictionary[dictionaryIndex].first;

					currentInstruction[mismatchLocation] = singleCharFlip(currentInstruction[mismatchLocation]);

					decompressedProgram += currentInstruction; 
					previousInstruction = currentInstruction;

					lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::TWO_BIT_CM):
				{
					int mismatchLocation{};

					mismatchLocation = stoi(cSerializedProgram.substr(index, 5), nullptr, 2);
					//Mismatch Location captured
					index += 5;

					dictionaryIndex = stoi(cSerializedProgram.substr(index, 4), nullptr, 2);
					//Dictionary Index captured
					index += 4;

					dictionaryEntry
						= currentInstruction
						= dictionary[dictionaryIndex].first;

					currentInstruction[mismatchLocation] = singleCharFlip(currentInstruction[mismatchLocation]);
					currentInstruction[mismatchLocation+1] = singleCharFlip(currentInstruction[mismatchLocation+1]);

					decompressedProgram += currentInstruction; 
					previousInstruction = currentInstruction;

					lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::FOUR_BIT_CM):
				{
					int mismatchLocation{};

					mismatchLocation = stoi(cSerializedProgram.substr(index, 5), nullptr, 2);
					//Mismatch Location captured
					index += 5;

					dictionaryIndex = stoi(cSerializedProgram.substr(index, 4), nullptr, 2);
					//Dictionary Index captured
					index += 4;

					dictionaryEntry
						= currentInstruction
						= dictionary[dictionaryIndex].first;

					currentInstruction[mismatchLocation] = singleCharFlip(currentInstruction[mismatchLocation]);
					currentInstruction[mismatchLocation + 1] = singleCharFlip(currentInstruction[mismatchLocation + 1]);
					currentInstruction[mismatchLocation + 2] = singleCharFlip(currentInstruction[mismatchLocation + 2]);
					currentInstruction[mismatchLocation + 3] = singleCharFlip(currentInstruction[mismatchLocation + 3]);

					decompressedProgram += currentInstruction;
					previousInstruction = currentInstruction;

					lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::TWO_BIT_AM):
				{
					int mismatchLocation_First{};
					int mismatchLocation_Second{};

					mismatchLocation_First = stoi(cSerializedProgram.substr(index, 5), nullptr, 2);
					//Mismatch Location 1 captured
					index += 5;

					mismatchLocation_Second = stoi(cSerializedProgram.substr(index, 5), nullptr, 2);
					//Mismatch Location 2 captured
					index += 5;

					dictionaryIndex = stoi(cSerializedProgram.substr(index, 4), nullptr, 2);
					//Dictionary Index captured
					index += 4;

					dictionaryEntry
						= currentInstruction
						= dictionary[dictionaryIndex].first;

					currentInstruction[mismatchLocation_First] = singleCharFlip(currentInstruction[mismatchLocation_First]);
					currentInstruction[mismatchLocation_Second] = singleCharFlip(currentInstruction[mismatchLocation_Second]);

					decompressedProgram += currentInstruction;
					previousInstruction = currentInstruction;

					lineNumber++;
					break;
				}

				//####################################################################################################
				case (COMPRESSTION_TYPE::DIRECT):
				{

					dictionaryIndex = stoi(cSerializedProgram.substr(index, 4), nullptr, 2);
					//Dictionary Index captured
					index += 4;

					currentInstruction = dictionary[dictionaryIndex].first;

					decompressedProgram += currentInstruction;
					previousInstruction = currentInstruction;

					lineNumber++;
					break;
				}
			}
		}

	}

public:

	void showCompressed()
	{
		//Print Compressed Code
		//####################################################################################################
		string temp{};

		for (const auto& instruction : compressedProgram)
			temp = temp + instruction;

		int c = 0;
		int itr = 0;

		while (itr != temp.length())
		{
			if (temp[itr] == '-') { itr++; continue; }
			cout << temp[itr++];
			c++;
			if (c == 32)
			{
				cout << endl;
				c = 0;
			}
		}

		while (c != 32)
		{
			cout << 0; c++;
		}
		//####################################################################################################

		cout << endl<<"xxxx" << endl;

		//Print Dictionary
		//####################################################################################################
		for (const auto& entry : dictionary)
			cout << entry.first << endl;
		//####################################################################################################
	}

	string prepareCompressed()
	{
		//Prepare Compressed Code
		//####################################################################################################
		string temp{};
		std::stringstream buffer;

		for (const auto& instruction : compressedProgram)
			temp = temp + instruction;

		int c = 0;
		int itr = 0;

		while (itr != temp.length())
		{
			if (temp[itr] == '-') { itr++; continue; }
			buffer << temp[itr++];
			c++;
			if (c == 32)
			{
				buffer << endl;
				c = 0;
			}
		}

		while (c != 32)
		{
			buffer << 0; c++;
		}
		//####################################################################################################

		buffer << endl << "xxxx" << endl;

		//Prepare Dictionary
		//####################################################################################################
		for (const auto& entry : dictionary)
			buffer << entry.first << endl;
		//####################################################################################################

		return buffer.str();
	}

	string prepareDeCompressed()
	{
		std::stringstream buffer;

		//Print Compressed Code
		//####################################################################################################

		int c = 0;
		int itr = 0;

		while (itr != decompressedProgram.length())
		{
			buffer << decompressedProgram[itr++];
			c++;
			if (c == 32)
			{
				buffer << endl;
				c = 0;
			}
		}
		//####################################################################################################

		return buffer.str();
	}

	void showDeCompressed()
	{
		//Prepare Deompressed Code
		//####################################################################################################

		int c = 0;
		int itr = 0;
		int l=1;

		while (itr != decompressedProgram.length())
		{
			cout << decompressedProgram[itr++];
			c++;
			if (c == 32)
			{
				cout << endl;
				c = 0;
			}
		}
		//####################################################################################################

	}

	compressUtil(const string pInput, const string pOutput, const bool isToCompress = true)
	{
	
		if (isToCompress)
		{
			prepareDictionary(pInput);
			compressProgam();
		}
		else
		{
			retrieveDictionary(pInput);
			decompressFile();
		}

	}

	void writeToFile(string pOutput, bool isWriteCompressed)
	{
		std::ofstream ofsWriter(pOutput);
		if (isWriteCompressed)
		{
			ofsWriter << prepareCompressed();
			ofsWriter.close();
		}
		else
		{
			ofsWriter << prepareDeCompressed();
			ofsWriter.close();
		}
	}
};

int main(int argc, char* argv[])
{
	if (argc != 2) return 0;

	const std::string inputFile_ToCompress = "original.txt";
	const std::string outputFile_Compressed = "cout.txt";
	const std::string inputFile_ToDecompress = "compressed.txt";
	const std::string outputFile_Decompressed = "dout.txt";

	if (stoi(argv[1]) == 1)
	{
		compressUtil CompUtil(inputFile_ToCompress, outputFile_Compressed, true); 
		//CompUtil.showCompressed();
		CompUtil.writeToFile(outputFile_Compressed, true);

	}
	else
	{
		compressUtil CompUtil(inputFile_ToDecompress, outputFile_Decompressed, false);
		//CompUtil.showDeCompressed();
		CompUtil.writeToFile(outputFile_Decompressed, false);
	}

	//int t{};
	//cin >> t;

	return 0;
}