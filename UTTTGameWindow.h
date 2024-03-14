/* Author: Hanuman Chu
 * 
 * Declares Ultimate Tic Tac Toe UTTTGameWindow class which has both singleplayer and multiplayer modes and allows for saving/loading games
 */
#ifndef UTTT_GAME_WINDOW_H
#define UTTT_GAME_WINDOW_H

#include "UTTTNet.h"
#include "NeuralNetwork.hpp"
#include "UTTTGameState.h"
#include "MCTS.hpp"

#include <windows.h>
#include <vector>

class UTTTGameWindow {
public:
	/**
	 * @brief Constructs new game window for Ultimate Tic Tac Toe and enters window event loop
	 */
	UTTTGameWindow();
	
	/**
	 * @brief Cleans up memory
	 */
	~UTTTGameWindow();
	
	//Prevents copying windows
	UTTTGameWindow(const UTTTGameWindow& OTHER) = delete;
	UTTTGameWindow& operator=(const UTTTGameWindow& OTHER) = delete;
	
	/**
	 * @brief Starts a new game
	 */
	void newGame();
	
	/**
	 * @brief Creates a popup asking the user to pick a file and loads that file
	 * @throws invalid_argument throws if user doesn't choose a valid file
	 */
	void loadGame();
	
	/**
	 * @brief Paints the entire window
	 * @param hdc device context handler
	 */
	void paintWindow(HDC hdc);
	
	/**
	 * @brief Toggles whether the human player icon is X or O
	 */
	void toggleIcon();
	
	/**
	 * @brief Toggles whether the game is in multiplayer or singleplayer mode
	 */
	void toggleMultiplayer();
	
	/**
	 * @brief Set the number of simulations to the given number
	 * @param SIMULATIONS number of simulations to do in MCTS
	 */
	void setSimulations(const int SIMULATIONS);
	
	/**
	 * @brief If click corresponds to a valid move and it is the human player's turn, applies move and returns true
	 * @param X x position of click
	 * @param Y y position of click
	 * @return true if move played, false otherwise
	 */
	bool handleClick(const int X, const int Y);
	
	/**
	 * @brief Creates and launches input window
	 */
	void createInputWindow();
	
	/**
	 * @brief Creates a popup asking the user to pick a file name and saves game state there
	 * @throws invalid_argument throws if game hasn't started or user doesn't choose a valid file
	 */
	void saveGame();
	
	/**
	 * @brief Returns true if the current game state was saved, false otherwise
	 * @return true if the current game state was saved, false otherwise
	 */
	bool saved();
	
	/**
	 * @brief Runs MCTS to find move for the computer to play, then sets the computer move to the resulting move
	 * @param lpParam pointer to game window
	 * @return zero
	 */
	static DWORD WINAPI calculateMove(LPVOID lpParam);
private:
	/**
	 * @brief window instance handler for main window and input window
	 */
	HINSTANCE mHInstance, mHInstanceInput;
	/**
	 * @brief window handler for main window and input window
	 */
	HWND mHWnd, mHWndInput;
	/**
	 * @brief MCTS tree used to find game moves
	 */
	MCTS<UTTTNet, UTTTGameState> mMCTS;
	/**
	 * @brief number of simulations the computer should do in MCTS
	 */
	int mSimulations;
	/**
	 * @brief whether the current game state has been saved
	 */
	bool mSaved;
	/**
	 * @brief top menu handler
	 */
	HMENU mMenuBar;
	/**
	 * @brief shared pointer to displayed game state
	 */
	UTTTGameState mGameState;
	/**
	 * @brief whether a game has started
	 */
	bool mStarted;
	/**
	 * @brief the icon of the human player with a 0 for X and a 1 for O
	 */
	int mHumanIcon;
	/**
	 * @brief whether the game is in singleplayer or multiplayer mode
	 */
	bool mMultiplayer;
	/**
	 * @brief AI thread handler
	 */
	HANDLE mComputer;
	/**
	 * @brief move the computer has decided on or -1 if computer hasn't decided or -2 if it isn't the computer's turn
	 */
	int mComputerMove;
	/**
	 * @brief rectangle which represents the entire window
	 */
	RECT mWindow;
	/**
	 * @brief rectangle which represents the top bar where titles are displayed
	 */
	RECT mTopBar;
	/**
	 * @brief rectangle which represents the game rectangle where the game is shown
	 */
	RECT mGameRect;
	/**
	 * @brief size of space between 3 by 3 boards and between 3 by 3 boards and the edges of the game rectangle
	 */
	int mBoardSpacerLength;
	/**
	 * @brief size of mini board's 3 by 3 boards' sides
	 */
	int mMiniBoardSideLength;
	/**
	 * @brief width of line between mini board's 3 by 3 boards
	 */
	int mMiniBoardLineWidth;
	/**
	 * @brief size of space between close edge of mini board's 3 by 3 boards and line seperating them from other 3 by 3 boards
	 */
	int mMiniBoardLineOffset;
	/**
	 * @brief size of individual cells' sides
	 */
	int mBoardSideLength;
	/**
	 * @brief width of line between individual cells
	 */
	int mBoardLineWidth;
	/**
	 * @brief size of space between far edge of individual cells and the line seperating them from other individual cells
	 */
	int mBoardLineOffset;
	/**
	 * @brief title font
	 */
	HFONT mTitleFont;
	/**
	 * @brief mini board font
	 */
	HFONT mMiniBoardFont;
	/**
	 * @brief board font
	 */
	HFONT mBoardFont;
	
	/**
	 * @brief Returns X if type 1, O if type 2, and an empty string otherwise
	 * @param TYPE type of cell
	 * @return X if type 1, O if type 2, and an empty string otherwise
	 */
	string mGetIcon(const int TYPE);
	
	/**
	 * @brief Adds menu bar to window
	 */
	void mAddMenu();
	
	/**
	 * @brief Displays title of window
	 * @param hdc device context handler
	 * @param TITLE title to display
	 */
	void mDisplayTitle(HDC hdc, const string TITLE);
	
	/**
	 * @brief Displays the current game state
	 * @param hdc device context handler
	 */
	void mDisplayGameState(HDC hdc);
	
	/**
	 * @brief Returns true if the window is still running, false otherwise  
	 * @return true if the window is still running, false otherwise 
	 */
	bool mProcessingMessages();
	
	/**
	 * @brief Stops the computer's thought process
	 */
	void mFinishThinking();
	
	/**
	 * @brief Creates a thread for the computer to think of a move
	 */
	void mFindComputerMove();
};

/**
 * @brief Processes messages sent to main window
 * @param hWnd window handler
 * @param uMsg message to process
 * @param wParam additional message information
 * @param lParam additional message information
 * @return result of message processing
 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * @brief Processes messages sent to input window
 * @param hWnd window handler
 * @param uMsg message to process
 * @param wParam additional message information
 * @param lParam additional message information
 * @return result of message processing
 */
LRESULT CALLBACK InputWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif