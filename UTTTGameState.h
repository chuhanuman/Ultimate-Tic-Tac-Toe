/* Author: Hanuman Chu
 * 
 * Declares Ultimate Tic Tac Toe GameState class
 */
#ifndef UTTT_GAME_STATE_H
#define UTTT_GAME_STATE_H

#include <string>
#include <vector>

const unsigned int BOARD_SIDE_LENGTH = 9;
const unsigned int MINI_BOARD_SIDE_LENGTH = 3;
const unsigned int BOARD_ARRAY_SIZE = BOARD_SIDE_LENGTH * BOARD_SIDE_LENGTH * sizeof(unsigned int);
const unsigned int MINI_BOARD_ARRAY_SIZE = MINI_BOARD_SIDE_LENGTH * MINI_BOARD_SIDE_LENGTH * sizeof(unsigned int);

class UTTTGameState {
public:
	/**
	 * @brief Constructor which initializes a new game
	 */
	UTTTGameState();
	
	/**
	 * @brief Constructor for a new UTTTGameState given a previous board, previous mini board, previous move and next player
	 * @param BOARD previous board
	 * @param MINI_BOARD previous mini board
	 * @param MOVE the previous move
	 * @param PLAYER the next player
	 */
    UTTTGameState(const unsigned int BOARD[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH],const unsigned int MINI_BOARD[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH],const int MOVE,const unsigned int PLAYER);
	
	/**
	 * @brief Returns game state with information loaded from a given file path
	 * @param FILE_PATH file path to load game state from
	 * @return game state with loaded info
	 * @throws invalid_argument if file does not exist or is not in the correct format
	 */
    static UTTTGameState loadState(const std::string FILE_PATH);
	
    /**
	 * @brief Returns child game state based on playing a given move
	 * @param MOVE next move
	 * @return child game state
	 * @throws invalid_argument if move is invalid
	 */
    UTTTGameState getChild(const int MOVE) const;
	
	/**
	 * @brief Checks if a given move is valid
	 * @param MOVE move to check
	 * @return true if a move is valid, false otherwise
	 */
    bool isValid(const int MOVE) const;
	
	/**
	 * @brief Returns a string which contains the information needed to recreate this object
	 * @return the information needed to recreate this object
	 */
	std::string getKey() const;
	
	/**
	 * @brief Returns a std::vector of pairs of boards and move probabilities generated from the current board and the given move probabilities' symmetric equivalents
	 * @param PROBS std::vector of move probabilities
	 * @return pairs of boards and move probabilities generated from the current board and the given move probabilities' symmetric equivalents
	 * @throws invalid_argument if PROBS' size is less than the board's size
	 */
	std::vector<std::pair<std::vector<float>, std::vector<float>>> getSymmetries(const std::vector<float> PROBS) const;
	
	/**
	 * @brief Saves this object's game information in an easy to read format at a given file path
	 * @param FILE_PATH file path to save UTTTGameState in 
	 * @throws invalid_argument if there was an error in writing to the file
	 */
    void saveState(const std::string FILE_PATH) const;
	
	/**
	 * @brief Returns std::vector with all valid moves
	 * @return all valid moves
	 */
    std::vector<int> getValidMoves() const;
	
	/**
	 * @brief Returns the current board in a single row, with each row in the board after the previous
	 * @return current board
	 */
	std::vector<float> getBoard() const;
	
	/**
	 * @brief Returns the current mini board in a single row, with each row in the mini board after the previous
	 * @return current mini board
	 */
	std::vector<float> getMiniBoard() const;
	
	/**
	 * @brief Returns the next player
	 * @return next player with 0 if X and 1 if Y
	 */
	unsigned int getNextPlayer() const;
	
	/**
	 * @brief Returns the end state of this object 
	 * @return end state of this object with a 0 if X won, 1 if O won, 2 if the game has not ended, and 3 if the game is a tie
	 */
	unsigned int getEnd() const;
private:
	/**
	 * @brief holds the state of the overall board with a 0 for X, 1 for O, and 2 for empty cells
	 */
    unsigned int mBoard[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH];
	/**
	 * @brief holds the state of the 9 3 by 3 boards with a 0 for X, 1 for O, 2 for unfinished boards, and 3 for ties
	 */
    unsigned int mMiniBoard[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH];
    /**
	 * @brief holds the move made before this game state. -1 if no moves have been made
	 */
	int mPrevMove;
    /**
	 * @brief the next player with a 0 for X and a 1 for O
	 */
	unsigned int mNextPlayer;
	/**
	 * @brief holds the valid moves that can be made from this object
	 */
	std::vector<int> mValidMoves;
    /**
	 * @brief the end state of this object with a 0 if X won, 1 if O won, 2 if the game has not ended, and 3 if the game is a tie
	 */
	unsigned int mEnd;
	
	/**
	 * @brief Initializes a new UTTTGameState with the previous move and next player
	 * @param MOVE the previous move
	 * @param PLAYER the next player
	 */
    void mInit(const int MOVE, const unsigned int PLAYER);
	
	/**
	 * @brief Initializes validMoves with all valid moves
	 */
    void mGenerateValidMoves();
	
	/**
	 * @brief Returns a std::vector with every empty cell on the board
	 * @return all empty cells 
	 */
    std::vector<int> mGetAllEmpty() const;
	
	/**
	 * @brief Applies a given move for a given player on the given board and mini board
	 * @param board board to edit
	 * @param miniBoard mini board to edit
	 * @param MOVE the current move
	 * @param PLAYER the player making the move 
	 */
    static void mEditBoard(unsigned int (&board)[BOARD_SIDE_LENGTH][BOARD_SIDE_LENGTH], unsigned int (&miniBoard)[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH], const int MOVE, const unsigned int PLAYER);
	
	/**
	 * @brief Returns the winner of a given 3 by 3 board with 0 for X, 1 for O, 2 for no winner, and 3 for a tie
	 * @param MINI_BOARD 3 by 3 board
	 * @return winner of board with 0 for X, 1 for O, 2 for no winner, and 3 for a tie
	 */
	static unsigned int mFindWinner(const unsigned int (&MINI_BOARD)[MINI_BOARD_SIDE_LENGTH][MINI_BOARD_SIDE_LENGTH]);
};

#endif