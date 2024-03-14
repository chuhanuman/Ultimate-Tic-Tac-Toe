/* Author: Hanuman Chu
 * 
 * Defines Ultimate Tic Tac Toe UTTTGameWindow class which has both singleplayer and multiplayer modes and allows for saving/loading games
 */
#include "UTTTGameWindow.h"

#include <stdexcept>
#include <CommCtrl.h>

#define IDM_FILE_NEW 0
#define IDM_FILE_LOAD 1
#define IDM_FILE_SAVE 2
#define IDM_FILE_QUIT 3
#define IDM_TOGGLE_ICON 4
#define IDM_TOGGLE_MULTIPLAYER 5
#define IDM_SET_SIMULATIONS 6

UTTTGameWindow::UTTTGameWindow() : mMCTS(NeuralNetwork<UTTTNet>(0), 0) {
	mHInstance = GetModuleHandle(NULL);
	mHInstanceInput = GetModuleHandle(NULL);
	mStarted = false;
	mSaved = false;
	mComputer = NULL;
	mComputerMove = -1;
	mSimulations = 50;
	mWindow = {0,0,700,850};
	mTopBar = {50,25,650,125};
	mGameRect = {20,120,680,780};
	mBoardSpacerLength = (mGameRect.right - mGameRect.left) / 22;
	mMiniBoardSideLength = mBoardSpacerLength * 6;
	mMiniBoardLineWidth = mBoardSpacerLength / 5;
	mMiniBoardLineOffset = mBoardSpacerLength / 2 - mMiniBoardLineWidth / 2;
	mBoardSideLength = mBoardSpacerLength * 2;
	mBoardLineWidth = mBoardSpacerLength / 15;
	mBoardLineOffset = mBoardSideLength - mBoardLineWidth / 2;
	
	WNDCLASS wndInputClass = {};
	wndInputClass.lpszClassName = TEXT("Input Class");
    wndInputClass.lpfnWndProc = InputWindowProc;
    wndInputClass.hInstance = mHInstanceInput;
	RegisterClass(&wndInputClass);
	
	mHWndInput = NULL;
	
	WNDCLASS wndClass = {};
	wndClass.lpszClassName = TEXT("Game Window Class");
    wndClass.lpfnWndProc = WindowProc;
    wndClass.hInstance = mHInstance;
	RegisterClass(&wndClass);
	
	mHWnd = CreateWindowEx(
        0,
        TEXT("Game Window Class"),
        TEXT("Ultimate Tic Tac Toe"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU ,
        mWindow.left, mWindow.top, mWindow.right-mWindow.left, mWindow.bottom-mWindow.top,
        NULL,
        NULL,
        mHInstance,
        NULL
    );
	
	SetWindowLongPtr(mHWnd, GWLP_USERDATA, (LONG_PTR)this);
	
	ShowWindow(mHWnd, SW_SHOWNORMAL);
	
	mTitleFont = CreateFont(
		64, 28,
		0, 0, 
		0, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
		NULL
	);
	mMiniBoardFont = CreateFont(
		176, 77,
		0, 0, 
		0, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
		NULL
	);
	mBoardFont = CreateFont(
		32, 14,
		0, 0, 
		0, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
		NULL
	);
	
	mAddMenu();
	
	mHumanIcon = 1;
	toggleIcon();
	mMultiplayer = true;
	toggleMultiplayer();
	
	NeuralNetwork<UTTTNet> mNN(81);
	
	if (!mNN.load("models/verifiedbest.pt")) {
		MessageBoxA(mHWnd, "Model for neural network did not load correctly from models/verifiedbest.pt. Make sure that that file exists.", NULL, MB_OK | MB_ICONERROR);
	}
	
	mMCTS = MCTS<UTTTNet, UTTTGameState>(mNN, mSimulations);
	
	while (mProcessingMessages()) {
		if (mComputerMove != -1) {
			mGameState = mGameState.getChild(mComputerMove);
			mSaved = false;
			InvalidateRect(mHWnd, NULL, TRUE);
			mFinishThinking();
		}
		
		Sleep(200);
	}
}

UTTTGameWindow::~UTTTGameWindow() {
	mFinishThinking();
	
	UnregisterClass(TEXT("Game Window Class"), mHInstance);
	UnregisterClass(TEXT("Input Class"), mHInstanceInput);
}

void UTTTGameWindow::newGame() {
	mFinishThinking();
    mGameState = UTTTGameState();
	mSaved = false;
	mStarted = true;
	
	if (!mMultiplayer && mHumanIcon == 1) {
		mFindComputerMove();
	}
}

void UTTTGameWindow::loadGame() {
	mFinishThinking();
	
	char filePath[MAX_PATH] = "";
	OPENFILENAMEA openFileName;
	ZeroMemory(&openFileName, sizeof(openFileName));
	openFileName.lStructSize = sizeof(openFileName);
	openFileName.lpstrFile = filePath;
	openFileName.nMaxFile = MAX_PATH;
	openFileName.Flags = OFN_FILEMUSTEXIST;
	
	if (!GetOpenFileNameA(&openFileName)) {
		return;
	}
	
	try {
		mGameState = UTTTGameState::loadState(filePath);
	} catch (std::invalid_argument ex) {
		throw std::invalid_argument(ex.what());
	}
	
	mStarted = true;
	if (!mMultiplayer && mGameState.getNextPlayer() != mHumanIcon) {
		mFindComputerMove();
	}
}

void UTTTGameWindow::paintWindow(HDC hdc) {
	FillRect(hdc, &mWindow, (HBRUSH)WHITE_BRUSH); 
	SetBkMode(hdc, TRANSPARENT);
	if (!mStarted) {
		mDisplayTitle(hdc, "Ultimate Tic Tac Toe");
	} else {
		if (mGameState.getEnd() == 3) {
			mDisplayTitle(hdc, "Tie");
		} else if (mGameState.getEnd() == 2) {
			mDisplayTitle(hdc, mGetIcon(mGameState.getNextPlayer()) + " playing");
		} else {
			mDisplayTitle(hdc, mGetIcon(1-mGameState.getNextPlayer()) + " won");
			mFinishThinking();
		}
		mDisplayGameState(hdc);
	}
}

void UTTTGameWindow::toggleIcon() {
	mHumanIcon = 1 - mHumanIcon;
	
	HMENU playerIconMenu = GetSubMenu(GetSubMenu(mMenuBar, 1), 0);
	if (mHumanIcon == 0) {
		EnableMenuItem(playerIconMenu, 0, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(playerIconMenu, 1, MF_BYPOSITION | MF_ENABLED);
	} else {
		EnableMenuItem(playerIconMenu, 0, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(playerIconMenu, 1, MF_BYPOSITION | MF_GRAYED);
	}
	
	if (mStarted && mGameState.getNextPlayer() != mHumanIcon) {
		mFindComputerMove();
	} else {
		mFinishThinking();
	}
}

void UTTTGameWindow::toggleMultiplayer() {
	mMultiplayer = !mMultiplayer;
	
	HMENU gameSettingsMenu = GetSubMenu(mMenuBar, 1);
	HMENU multiplayerMenu = GetSubMenu(gameSettingsMenu, 2);
	if (mMultiplayer) {
		mFinishThinking();
		
		EnableMenuItem(gameSettingsMenu, 0, MF_BYPOSITION | MF_GRAYED);
		
		EnableMenuItem(multiplayerMenu, 0, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(multiplayerMenu, 1, MF_BYPOSITION | MF_ENABLED);
	} else {
		if (mStarted && mGameState.getNextPlayer() != mHumanIcon) {
			mFindComputerMove();
		}
		
		EnableMenuItem(gameSettingsMenu, 0, MF_BYPOSITION | MF_ENABLED);
		
		EnableMenuItem(multiplayerMenu, 0, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(multiplayerMenu, 1, MF_BYPOSITION | MF_GRAYED);
	}
}

void UTTTGameWindow::setSimulations(const int SIMULATIONS) {
	mSimulations = SIMULATIONS;
	mMCTS.setSimulations(mSimulations);
}

bool UTTTGameWindow::handleClick(int x, int y) {
	if (mStarted && (mGameRect.left <= x && x <= mGameRect.right) && (mGameRect.top <= y && y <= mGameRect.bottom) && (mMultiplayer || mHumanIcon == mGameState.getNextPlayer())) {
		bool outOfBounds = false;
		x -= mGameRect.left;
		y -= mGameRect.top;
		int i = x % (mMiniBoardSideLength + mBoardSpacerLength);
		int j = y % (mMiniBoardSideLength + mBoardSpacerLength);
		if (i < mBoardSpacerLength || j < mBoardSpacerLength) {
			outOfBounds = true;
		}
		i = 3 * (x / (mMiniBoardSideLength + mBoardSpacerLength)) + (i - mBoardSpacerLength) / mBoardSideLength;
		j = 3 * (y / (mMiniBoardSideLength + mBoardSpacerLength)) + (j - mBoardSpacerLength) / mBoardSideLength;
		
		int move = j * 9 + i;
		if (!outOfBounds && mGameState.isValid(move)) {
			mGameState = mGameState.getChild(move);
			mSaved = false;
			
			if (!mMultiplayer) {
				mFindComputerMove();
			}
			
			return true;
		} else {
			MessageBeep(MB_ICONEXCLAMATION);
		}
	}
	return false;
}

void UTTTGameWindow::createInputWindow() {
	if (mHWndInput != NULL) {
		ShowWindow(mHWndInput, SW_SHOWNORMAL);;
	}
	
	mHWndInput = CreateWindowEx(
		0,
		TEXT("Input Class"), 
		TEXT("Simulations"), 
		WS_POPUP | WS_BORDER,
		25, 375, 400, 100,
		mHWnd, 
		NULL,
		NULL,
		NULL
	);
	
	SetWindowLongPtr(mHWndInput, GWLP_USERDATA, (LONG_PTR)this);

	HWND hWndTrackBar = CreateWindowEx(
		0,
		TRACKBAR_CLASS, 
		NULL, 
		WS_CHILD | WS_VISIBLE,
		10, 20, 380, 30,
		mHWndInput, 
		NULL,
		NULL,
		NULL
	);
	
	SendMessageW(hWndTrackBar, TBM_SETRANGE, TRUE, MAKELONG(50, 1000)); 
	SendMessageW(hWndTrackBar, TBM_SETTIC, 0, 50);
	SendMessageW(hWndTrackBar, TBM_SETTIC, 0, 100);
	SendMessageW(hWndTrackBar, TBM_SETTIC, 0, 500);
	SendMessageW(hWndTrackBar, TBM_SETTIC, 0, 1000);
	SendMessageW(hWndTrackBar, TBM_SETPOS, TRUE, mSimulations);
	
	HWND hWndButton = CreateWindowEx(
		0,
		TEXT("BUTTON"), 
		TEXT("Move the slider"), 
		WS_CHILD | WS_VISIBLE | BS_CENTER,
		10, 60, 380, 30,
		mHWndInput, 
		NULL,
		NULL,
		NULL
	);
	
	ShowWindow(mHWndInput, SW_SHOWNORMAL);
}

void UTTTGameWindow::saveGame() {
	if (!mStarted) {
		throw std::invalid_argument("You have to be in a game first.");
	}
	
	char filePath[MAX_PATH] = "";
	OPENFILENAMEA openFileName;
	ZeroMemory(&openFileName, sizeof(openFileName));
	openFileName.lStructSize = sizeof(openFileName);
	openFileName.lpstrFile = filePath;
	openFileName.nMaxFile = MAX_PATH;
	openFileName.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	
	if (!GetSaveFileNameA(&openFileName)) {
		return;
	}
	
	try {
		mGameState.saveState(filePath);
		mSaved = true;
	} catch (std::invalid_argument ex) {
		throw std::invalid_argument(ex.what());
	}
}

bool UTTTGameWindow::saved() {
	if (!mStarted) {
		return true;
	} 
	return mSaved;
}

DWORD WINAPI UTTTGameWindow::calculateMove(LPVOID lpParam) {
	UTTTGameWindow* pUTTTGameWindow = (UTTTGameWindow*)lpParam;
	
	std::vector<float> probs = pUTTTGameWindow->mMCTS.getBestMove(pUTTTGameWindow->mGameState);
	
	int move = -1;
	for (int i=0;i<probs.size();i++) {
		if (probs.at(i) == 1.0f) {
			move = i;
			break;
		}
	}
	
	pUTTTGameWindow->mComputerMove = move;
	return 0;
}

string UTTTGameWindow::mGetIcon(int cell) {
	switch (cell) {
		case 0:
			return "X";
		case 1:
			return "O";
		case 2:
			return "";
	}
	return "";
}

void UTTTGameWindow::mAddMenu() {
	mMenuBar = CreateMenu();
	HMENU fileMenu = CreateMenu(), gameSettingsMenu = CreateMenu();
	HMENU playerIconMenu = CreateMenu(), multiplayerMenu = CreateMenu();
	
	AppendMenu(fileMenu, MF_STRING, IDM_FILE_NEW, TEXT("New game"));
	AppendMenu(fileMenu, MF_STRING, IDM_FILE_LOAD, TEXT("Load game"));
	AppendMenu(fileMenu, MF_STRING, IDM_FILE_SAVE, TEXT("Save game"));
	AppendMenu(fileMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(fileMenu, MF_STRING, IDM_FILE_QUIT, TEXT("Exit"));
	
	AppendMenu(playerIconMenu, MF_STRING, IDM_TOGGLE_ICON, TEXT("X"));
	AppendMenu(playerIconMenu, MF_STRING, IDM_TOGGLE_ICON, TEXT("O"));
	
	AppendMenu(multiplayerMenu, MF_STRING, IDM_TOGGLE_MULTIPLAYER, TEXT("Player vs Player"));
	AppendMenu(multiplayerMenu, MF_STRING, IDM_TOGGLE_MULTIPLAYER, TEXT("Player vs. Computer"));
	
	AppendMenu(gameSettingsMenu, MF_POPUP, (UINT_PTR)playerIconMenu, TEXT("Set Player Icon"));
	AppendMenu(gameSettingsMenu, MF_POPUP, IDM_SET_SIMULATIONS, TEXT("Set Simulations"));
	AppendMenu(gameSettingsMenu, MF_POPUP, (UINT_PTR)multiplayerMenu, TEXT("Set Multiplayer"));
	
	AppendMenu(mMenuBar, MF_POPUP, (UINT_PTR)fileMenu, TEXT("File"));
	AppendMenu(mMenuBar, MF_POPUP, (UINT_PTR)gameSettingsMenu, TEXT("Settings"));
	SetMenu(mHWnd, mMenuBar);
}

void UTTTGameWindow::mDisplayTitle(HDC hdc, string title) {
	SelectObject(hdc, mTitleFont);
	DrawTextA(hdc, title.c_str(), title.length(), &mTopBar, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void UTTTGameWindow::mDisplayGameState(HDC hdc) {
	HBRUSH yellowBrush = CreateSolidBrush(RGB(255, 255, 0));
	HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
	RECT cell;
	RECT line;
	int x, y, pos;
	
	for (int move:mGameState.getValidMoves()) {
		x = move % BOARD_SIDE_LENGTH;
		y = move / BOARD_SIDE_LENGTH;
		
		x = mGameRect.left + mBoardSideLength * x + mBoardSpacerLength * (x / MINI_BOARD_SIDE_LENGTH + 1);
		y = mGameRect.top + mBoardSideLength * y + mBoardSpacerLength * (y / MINI_BOARD_SIDE_LENGTH + 1);
		cell = {x, y, x + mBoardSideLength, y + mBoardSideLength};
		FillRect(hdc, &cell, yellowBrush);
	}
	
	SelectObject(hdc, mBoardFont);
	x = mGameRect.left + mBoardSpacerLength;
	std::vector<float> board = mGameState.getBoard();
	for (int i=0;i<BOARD_SIDE_LENGTH;i++) {
		y = mGameRect.top + mBoardSpacerLength;
		for (int j=0;j<BOARD_SIDE_LENGTH;j++) {
			cell = {x, y, x + mBoardSideLength, y + mBoardSideLength};
			DrawTextA(hdc, mGetIcon(board.at(j * BOARD_SIDE_LENGTH + i)).c_str(), mGetIcon(board.at(j * BOARD_SIDE_LENGTH + i)).length(), &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			
			y += mBoardSideLength;
			if (j % MINI_BOARD_SIDE_LENGTH == MINI_BOARD_SIDE_LENGTH - 1) {
				y += mBoardSpacerLength;
			}
		}
		x += mBoardSideLength;
		if (i % MINI_BOARD_SIDE_LENGTH == MINI_BOARD_SIDE_LENGTH - 1) {
			x += mBoardSpacerLength;
		}
	}
	
	SelectObject(hdc, mMiniBoardFont);
	x = mGameRect.left + mBoardSpacerLength;
	std::vector<float> miniBoard = mGameState.getMiniBoard();
	for (int i=0;i<MINI_BOARD_SIDE_LENGTH;i++) {
		y = mGameRect.top + mBoardSpacerLength;
		for (int j=0;j<MINI_BOARD_SIDE_LENGTH;j++) {
			cell = {x, y, x + mMiniBoardSideLength, y + mMiniBoardSideLength};
			DrawTextA(hdc, mGetIcon(miniBoard.at(j * MINI_BOARD_SIDE_LENGTH + i)).c_str(), mGetIcon(miniBoard.at(j * MINI_BOARD_SIDE_LENGTH + i)).length(), &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			
			line = {x + mBoardLineOffset, y, x + mBoardLineOffset + mBoardLineWidth, y + mMiniBoardSideLength};
			FillRect(hdc, &line, blackBrush);
			line = {x + mBoardLineOffset + mBoardSideLength, y, x + mBoardLineOffset + mBoardSideLength + mBoardLineWidth, y + mMiniBoardSideLength};
			FillRect(hdc, &line, blackBrush);
			line = {x, y + mBoardLineOffset, x + mMiniBoardSideLength, y + mBoardLineOffset + mBoardLineWidth};
			FillRect(hdc, &line, blackBrush);
			line = {x, y + mBoardLineOffset + mBoardSideLength, x + mMiniBoardSideLength, y + mBoardLineOffset + mBoardSideLength + mBoardLineWidth};
			FillRect(hdc, &line, blackBrush);
			
			y += mMiniBoardSideLength + mBoardSpacerLength;
		}
		x += mMiniBoardSideLength + mBoardSpacerLength;
	}
	
	pos = mMiniBoardSideLength + mBoardSpacerLength;
	for (int i=0;i<MINI_BOARD_SIDE_LENGTH - 1;i++) {
		line = {mGameRect.left + pos + mMiniBoardLineOffset, mGameRect.top, mGameRect.left + pos + mMiniBoardLineOffset + mMiniBoardLineWidth, mGameRect.bottom};
		FillRect(hdc, &line, blackBrush);
		line = {mGameRect.left, mGameRect.top + pos + mMiniBoardLineOffset, mGameRect.right, mGameRect.top + pos + mMiniBoardLineOffset + mMiniBoardLineWidth};
		FillRect(hdc, &line, blackBrush);
		
		pos += mBoardSpacerLength + mMiniBoardSideLength;
	}
	
	DeleteObject(yellowBrush);
	DeleteObject(blackBrush);
}

bool UTTTGameWindow::mProcessingMessages() {
	MSG msg = {};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		switch(msg.message) {
			case WM_QUIT:
				return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return true;
}

void UTTTGameWindow::mFinishThinking() {
	if (mComputer == NULL) {
		return;
	}
	
	DWORD computerResult;
	GetExitCodeThread(mComputer,&computerResult);
	if (computerResult == STILL_ACTIVE) {
		TerminateThread(mComputer, 0);
	}
	CloseHandle(mComputer);
	
	mComputer = NULL;
	mComputerMove = -1;
}

void UTTTGameWindow::mFindComputerMove() {
	mComputer = CreateThread(NULL, 0, calculateMove, this, 0, NULL);
	mComputerMove = -1;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UTTTGameWindow* utttGameWindow = (UTTTGameWindow*)(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	PAINTSTRUCT paintStruct;
	HDC hdc;
	int response;
	
	switch (uMsg)
	{
		case WM_CREATE:
			break;
		case WM_CLOSE:
			if (utttGameWindow->saved()) {
				DestroyWindow(hWnd);
				break;
			}
			
			response = MessageBoxA(hWnd, "Do you want to save the game first?", "Warning", MB_YESNOCANCEL | MB_ICONWARNING);
			
			switch (response) {
				case IDYES:
					SendMessage(hWnd, IDM_FILE_SAVE, 0, 0);
				case IDCANCEL:
					return 0;
				case IDNO:
					DestroyWindow(hWnd);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &paintStruct);
			utttGameWindow->paintWindow(hdc);
			EndPaint(hWnd, &paintStruct);
			break;
		case WM_COMMAND:
			switch (wParam) {
				//New game
				case IDM_FILE_NEW:
					utttGameWindow->newGame();
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				//Load game
				case IDM_FILE_LOAD:
					try {
						utttGameWindow->loadGame();
						InvalidateRect(hWnd, NULL, TRUE);
					} catch (std::invalid_argument ex) {
						MessageBoxA(hWnd, ex.what(), NULL, MB_OK | MB_ICONERROR);
					}
					break;
				//Save game
				case IDM_FILE_SAVE:
					try {
						utttGameWindow->saveGame();
					} catch (std::invalid_argument ex) {
						MessageBoxA(hWnd, ex.what(), NULL, MB_OK | MB_ICONERROR);
					}
					break;
				//Quit
				case IDM_FILE_QUIT:
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				case IDM_TOGGLE_ICON:
					utttGameWindow->toggleIcon();
					break;
				case IDM_TOGGLE_MULTIPLAYER:
					utttGameWindow->toggleMultiplayer();
					break;
				case IDM_SET_SIMULATIONS:
					utttGameWindow->createInputWindow();
					break;
			}
			break;
		case WM_LBUTTONDOWN:
			if (utttGameWindow->handleClick(LOWORD(lParam), HIWORD(lParam))) {
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
	}
	
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK InputWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	UTTTGameWindow* utttGameWindow = (UTTTGameWindow*)(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	int result;
	string message;
	
	switch (uMsg) {
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_HSCROLL:
			result = SendMessageW(FindWindowEx(hWnd, NULL, TRACKBAR_CLASS, NULL), TBM_GETPOS, 0, 0);
			message = "Set the number of simulations to " + to_string(result);
			SetWindowTextA(FindWindowEx(hWnd, NULL, TEXT("BUTTON"), NULL), message.c_str());
			break;
		case WM_COMMAND:	
			switch(wParam) {
				case BN_CLICKED:
					result = SendMessageW(FindWindowEx(hWnd, NULL, TRACKBAR_CLASS, NULL), TBM_GETPOS, 0, 0);
					utttGameWindow->setSimulations(result);
					utttGameWindow = nullptr;
					DestroyWindow(hWnd);
					break;
			}

            break;
	}
	
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}