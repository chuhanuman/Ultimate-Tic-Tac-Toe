/* Author: Hanuman Chu
 * 
 * Defines Ultimate Tic Tac Toe GameState class
 */
#include "UTTTGameState.h"

#include <algorithm>
#include <fstream>

UTTTGameState::UTTTGameState() {
	for (unsigned int i=0;i<MINI_BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<MINI_BOARD_SIDE_LENGTH;j++) {
			mMiniBoard[i][j] = 2;
		}
	}
	for (unsigned int i=0;i<BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<BOARD_SIDE_LENGTH;j++) {
			mBoard[i][j] = 2;
		}
	}
	mInit(-1, 0);
}

UTTTGameState::UTTTGameState(const unsigned int BOARD[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH], const unsigned int MINI_BOARD[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH], const int MOVE, const unsigned int PLAYER) {
	memcpy(this->mBoard, BOARD, BOARD_ARRAY_SIZE);
	memcpy(this->mMiniBoard, MINI_BOARD, MINI_BOARD_ARRAY_SIZE);
	mInit(MOVE,PLAYER);
}

UTTTGameState UTTTGameState::loadState(std::string filePath) {
	std::ifstream fin(filePath);
	if (fin.fail()) {
		throw std::invalid_argument("Could not open the file for reading.");
	}
	
	unsigned int miniBoard[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH];
	unsigned int board[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH];
	int prev_move = -1;
	unsigned int player;
	unsigned int counter = 0;
	int cell;
	while (counter < 92) {
		fin >> cell;
		if (fin.fail()) {
			fin.close();
			throw std::invalid_argument("Error in reading from file.");
		}
		if (counter < 81) {
			board[counter / BOARD_SIDE_LENGTH][counter % BOARD_SIDE_LENGTH] = cell;
		} else if (counter < 90) {
			miniBoard[(counter-BOARD_SIDE_LENGTH*BOARD_SIDE_LENGTH) / MINI_BOARD_SIDE_LENGTH][counter % MINI_BOARD_SIDE_LENGTH] = cell;
		} else if (counter < 91) {
			prev_move = cell;
		} else {
			player = cell;
		}
		counter++;
	}
	fin.close();
	
	return UTTTGameState(board, miniBoard, prev_move, player);
}

UTTTGameState UTTTGameState::getChild(const int MOVE) const {
	if (!isValid(MOVE)) {
		throw std::invalid_argument("Invalid move.");
	}
	
	unsigned int copyBoard[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH];
	memcpy(copyBoard, mBoard, BOARD_ARRAY_SIZE);
	unsigned int copyMiniBoard[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH];
	memcpy(copyMiniBoard, mMiniBoard, MINI_BOARD_ARRAY_SIZE);
	mEditBoard(copyBoard, copyMiniBoard, MOVE, mNextPlayer);
	
	return UTTTGameState(copyBoard, copyMiniBoard, MOVE, 1-mNextPlayer);
}

bool UTTTGameState::isValid(const int MOVE) const {
	return std::find(mValidMoves.begin(), mValidMoves.end(), MOVE) != mValidMoves.end();
}

std::string UTTTGameState::getKey() const {
	std::string key;
	
	for (unsigned int i=0;i<BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<BOARD_SIDE_LENGTH;j++) {
			key += (char)mBoard[i][j];
		}
	}
	key += (char)mPrevMove;
	
	return key;
}

std::vector<std::pair<std::vector<float>, std::vector<float>>> UTTTGameState::getSymmetries(const std::vector<float> PROBS) const {
	if (PROBS.size() < BOARD_SIDE_LENGTH * BOARD_SIDE_LENGTH) {
		throw std::invalid_argument("Input std::vector is too small.");
	}
	
	std::vector<std::pair<std::vector<float>, std::vector<float>>> symmetries;
	
	std::vector<float> curBoard = getBoard(), reflectedBoard = curBoard;
	std::vector<float> curProbs = PROBS, reflectedProbs = curProbs;
	
	for (int i=0;i<4;i++) {
		if (i != 0) {
			std::vector<float> originalBoard = curBoard;
			std::vector<float> originalProbs = curProbs;
			for (unsigned int j=0;j<BOARD_SIDE_LENGTH*BOARD_SIDE_LENGTH;j++) {
				int originalPos = (j % BOARD_SIDE_LENGTH) * BOARD_SIDE_LENGTH + (BOARD_SIDE_LENGTH - 1 - j / BOARD_SIDE_LENGTH);
				curBoard[j] = originalBoard[originalPos];
				curProbs[j] = originalProbs[originalPos];
			}
		}
		for (unsigned int j=0;j<BOARD_SIDE_LENGTH*BOARD_SIDE_LENGTH;j++) {
			int reflectedPos = (j / BOARD_SIDE_LENGTH) * BOARD_SIDE_LENGTH + (BOARD_SIDE_LENGTH - 1 - j % BOARD_SIDE_LENGTH);
			reflectedBoard[reflectedPos] = curBoard[j];
			reflectedProbs[reflectedPos] = curProbs[j];
		}
		
		symmetries.push_back({curBoard,curProbs});
		symmetries.push_back({reflectedBoard,reflectedProbs});
	}
	
	return symmetries;
}

void UTTTGameState::saveState(const std::string FILE_PATH) const {
	std::ofstream fout(FILE_PATH);
	
	if (fout.fail()) {
		fout.close();
		throw std::invalid_argument("Could not open the file for writing.");
	}
	
	for (unsigned int i=0;i<BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<BOARD_SIDE_LENGTH;j++) {
			fout << mBoard[i][j] << " ";
		}
		fout << '\n';
	}
	
	for (unsigned int i=0;i<MINI_BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<MINI_BOARD_SIDE_LENGTH;j++) {
			fout << mMiniBoard[i][j] << " ";
		}
		fout << '\n';
	}
	
	fout << mPrevMove << " " << mNextPlayer << " " << '\n';
	
	if (fout.fail()) {
		fout.close();
		throw std::invalid_argument("Error in writing to file.");
	}
	fout.close();
}

std::vector<int> UTTTGameState::getValidMoves() const {
	return mValidMoves;
}

std::vector<float> UTTTGameState::getBoard() const {
	std::vector<float> board;
	for (unsigned int i=0;i<BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<BOARD_SIDE_LENGTH;j++) {
			board.push_back(mBoard[i][j]);
		}
	}
	
	return board;
}

std::vector<float> UTTTGameState::getMiniBoard() const {
	std::vector<float> miniBoard;
	for (unsigned int i=0;i<MINI_BOARD_SIDE_LENGTH;i++) {
		for (unsigned int j=0;j<MINI_BOARD_SIDE_LENGTH;j++) {
			miniBoard.push_back(mMiniBoard[i][j]);
		}
	}
	
	return miniBoard;
}

unsigned int UTTTGameState::getNextPlayer() const {
	return mNextPlayer;
}

unsigned int UTTTGameState::getEnd() const {
	return mEnd;
}

void UTTTGameState::mInit(const int MOVE, const unsigned int PLAYER) {
	mPrevMove = MOVE;
	mNextPlayer = PLAYER;
	mEnd = mFindWinner(mMiniBoard);
	if (mEnd == 2) {
		mGenerateValidMoves();
	} else {
		mValidMoves = {};
	}
}

void UTTTGameState::mGenerateValidMoves() {
	mValidMoves = {};
	
	if (mPrevMove == -1) {
		mValidMoves = mGetAllEmpty();
		return;
	}
	
	unsigned int boardX = mPrevMove%3;
	unsigned int boardY = (mPrevMove/9)%3;
	if (mMiniBoard[boardY][boardX] != 2) {
		mValidMoves = mGetAllEmpty();
		return;
	}
	
	for (unsigned int i=boardX*3;i<3+boardX*3;i++) {
		for (unsigned int j=boardY*3;j<3+boardY*3;j++) {
			if (mBoard[j][i] == 2) {
				mValidMoves.push_back(j * BOARD_SIDE_LENGTH + i);
			}
		}
	}
}

std::vector<int> UTTTGameState::mGetAllEmpty() const {
	std::vector<int> moves;
	for (unsigned int boardX=0;boardX<3;boardX++) {
		for (unsigned int boardY=0;boardY<3;boardY++) {
			if (mMiniBoard[boardY][boardX] == 2) {
				for (unsigned int i=boardX*3;i<3+boardX*3;i++) {
					for (unsigned int j=boardY*3;j<3+boardY*3;j++) {
						if (mBoard[j][i] == 2) {
							moves.push_back(j * BOARD_SIDE_LENGTH + i);
						}
					}
				}
			}
		}
	}
	return moves;
}

void UTTTGameState::mEditBoard(unsigned int (&board)[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH], unsigned int (&miniBoard)[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH], const int MOVE, const unsigned int PLAYER) {
	board[MOVE / BOARD_SIDE_LENGTH][MOVE % BOARD_SIDE_LENGTH] = PLAYER;
	unsigned int boardX = (MOVE % BOARD_SIDE_LENGTH ) / 3;
	unsigned int boardY = (MOVE / BOARD_SIDE_LENGTH) / 3;
	
	unsigned int smallBoard[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH];
	for (unsigned int j=boardY*3;j<boardY*3+3;j++) {
		for (unsigned int i=boardX*3;i<boardX*3+3;i++) {
			smallBoard[j-boardY*3][i-boardX*3] = board[j][i];
		}
	}
	
	unsigned int result = mFindWinner(smallBoard);
	miniBoard[boardY][boardX] = result;
}

unsigned int UTTTGameState::mFindWinner(const unsigned int (&MINI_BOARD)[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH]) {
	int sums[5] = {0,0,0,0,0};
	bool tie = true;
	for (unsigned int i=0;i<3;i++) {
		int rowsum = 0;
		for (unsigned int j=0;j<3;j++) {
			unsigned int cell = MINI_BOARD[i][j];
			switch (cell) {
				case 0:
					rowsum++;
					sums[j]++;
					if (i == j) {
						sums[3]++;
					}
					if (i + j + 1 == 3) {
						sums[4]++;
					}
					break;
				case 1:
					rowsum--;
					sums[j]--;
					if (i == j) {
						sums[3]--;
					}
					if (i + j + 1 == 3) {
						sums[4]--;
					}
					break;
				case 2:
					tie = false;
					break;
			}
		}
		
		if (rowsum == 3) {
			return 0;
		} else if (rowsum == -3) {
			return 1;
		}
	}
	
	if (std::find(sums, sums + 5, 3) != sums + 5) {
		return 0;
	} else if (std::find(sums, sums + 5, -3) != sums + 5) {
		return 1;
	} else if (tie) {
		return 3;
	}
	return 2;
}