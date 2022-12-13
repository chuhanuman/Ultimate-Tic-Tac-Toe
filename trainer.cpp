/* Author: Hanuman Chu
 * 
 * Trains neural network with parameters from config file using a three step process: generating examples through self play, training on those examples, and testing trained network against previous network and keeping the better one
 */
#include "UTTTNet.h"
#include "NeuralNetwork.hpp"
#include "UTTTGameState.h"
#include "MCTS.hpp"

#include <iostream>
#include <limits>
#include <fstream>
#include <chrono>
#include <random>
using namespace std;

/**
 * @brief Displays move probabilties in an easy to read format
 * @param probs move probabilities to display
 */
void displayUTTTProbs(vector<float> probs) {
	int row = 0, col = 0;
	for (float chance:probs) {
		if (chance < 0.1f) {
			cout << "0" << (int)(chance * 100) << " ";
		} else if (chance == 1.0f) {
			cout << "!! ";
		} else {
			cout << (int)(chance * 100) << " ";
		}
		
		if (col == 8) {
			col = -1;
			if (row % 3 == 2) {
				cout << endl;
			}
			cout << endl;
			row++;
		} else if (col % 3 == 2) {
			cout << " ";
		}
		col++;
	}
}

/**
 * @brief Displays game board in an easy to read format
 * @param board game board to display
 */
void displayUTTTBoard(vector<float> board) {
	for (int i=0;i<board.size();i++) {
		cout << (int)board[i] << " ";
		if (i % 9 == 8) {
			cout << endl;
			if (i % 27 == 26) {
				cout << endl;
			}
		} else if (i % 3 == 2) {
			cout << " ";
		}
	}
}

int main(int argc, char* argv[]) {
	NeuralNetwork<UTTTNet> curNN(81), prevNN(81);
	if (argc >= 2) {
		if (!curNN.load(argv[1])) {
			cout << "ERROR: Starting current model did not load correctly from " << argv[1] << endl;
		}
		if (argc >= 3) {
			if (!prevNN.load(argv[2])) {
				cout << "ERROR: Starting previous model did not load correctly from " << argv[2] << endl;
			}
		}
	} else {
		cout << "WARNING: No model was passed." << endl;
	}
	
	ifstream fin("config.txt");
	
	if (fin.fail()) {
		cout << "FATAL: Config file did not load correctly" << endl;
		return 1;
	}
	
	vector<int> config;
	int iTemp;
	for (int i=0;i<9;i++) {
		fin >> iTemp;
		
		if (fin.fail()) {
			cout << "FATAL: Config file did not load correctly" << endl;
			return 1;
		}
		
		config.push_back(iTemp);
		fin.ignore(numeric_limits<streamsize>::max(), '\n');
	}
	fin.close();
	
	const int ITERATIONS = config.at(0), EPISODES = config.at(1), SIMULATIONS = config.at(2), GAMES = config.at(3), EXPLORATION_TURNS = config.at(4), BATCH_SIZE = config.at(5), LOAD_EXAMPLES = config.at(6), SKIP_TRAINING = config.at(7), DISPLAY_GAMES = config.at(8);
	
	MCTS<UTTTNet, UTTTGameState> curMCTS = MCTS<UTTTNet, UTTTGameState>(curNN, SIMULATIONS);
	MCTS<UTTTNet, UTTTGameState> prevMCTS = MCTS<UTTTNet, UTTTGameState>(prevNN, SIMULATIONS);
	default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
	for (int iteration=0;iteration<ITERATIONS;iteration++) {
		cout << "Starting iteration " << iteration << endl;
		chrono::steady_clock::time_point begin = chrono::steady_clock::now();
		
		vector<tuple<vector<float>, vector<float>, float>> examples;
		if (LOAD_EXAMPLES == 0 || iteration != 0) {
			ofstream fout("temp.ex");
			
			for (int episode=0;episode<EPISODES;episode++) {
				cout << "Starting episode " << episode << endl;
				cout << chrono::duration_cast<chrono::minutes>(chrono::steady_clock::now()-begin).count() << " minutes have passed" << endl;
				UTTTGameState gameState;
				vector<tuple<vector<float>, vector<float>, float>> curExamples; //examples in format {board, probs, value}
				
				int turns = 0;
				vector<float> probs;
				while (gameState.getEnd() == 2) {
					if (turns == 0) {
						for (int move = 0; move < 81; move++) {
							probs.push_back(move == 40 ? 1.0f : 0.0f);
						}
					} else if (turns < EXPLORATION_TURNS) {
						probs = curMCTS.getMoveProbs(gameState);
					} else {
						probs = curMCTS.getBestMove(gameState);
					}
					
					for (pair<vector<float>,vector<float>> symmetry:gameState.getSymmetries(probs)) {
						tuple<vector<float>, vector<float>, float> ex = make_tuple(symmetry.first, symmetry.second, -1.0f);
						curExamples.push_back(ex);
					}
					
					discrete_distribution<int> distribution(probs.begin(), probs.end());
					int move = distribution(generator);
					
					gameState = gameState.getChild(move);
					turns++;
				}
				float result;
				if (gameState.getEnd() != 3) {
					result = gameState.getEnd();
				} else {
					result = 0.5f;
				}
				
				for (tuple<vector<float>, vector<float>, float> ex:curExamples) {
					for (int i=0;i<81;i++) {
						fout << get<0>(ex).at(i) << " ";
						fout << get<1>(ex).at(i) << " ";
					}
					fout << result << endl;
					
					get<2>(ex) = result;
					examples.push_back(ex);
				}
				
				curMCTS.reset();
			}
			
			fout.close();
		}
		
		if (SKIP_TRAINING == 0 || iteration != 0) {
			if (!curNN.save("models/temp.pt")) {
				cout << "ERROR: Current model did not save correctly to models/temp.pt" << endl;
			}
			
			if (!prevNN.load("models/temp.pt")) {
				cout << "ERROR: Previous model did not load correctly from models/temp.pt" << endl;
			}
			
			for (int epoch=0;epoch<10 * ((LOAD_EXAMPLES > 0 && iteration == 0) ? LOAD_EXAMPLES : 1);epoch++) {
				if (LOAD_EXAMPLES > 0 && iteration == 0) {
					uniform_int_distribution<int> distribution(1,LOAD_EXAMPLES);
					examples = {};
					
					ifstream fin("examples/temp" + to_string(distribution(generator)) + ".ex");
					
					float fTemp;
					while (!fin.eof()) {
						tuple<vector<float>, vector<float>, float> example;
						
						for (int i=0;i<81;i++) {
							fin >> fTemp;
							get<0>(example).push_back(fTemp);
							fin >> fTemp;
							get<1>(example).push_back(fTemp);
						}
						fin >> get<2>(example);
						
						if (fin.fail()) {
							break;
						}
						
						examples.push_back(example);
					}
					
					fin.close();
				}
				
				cout << "Training with " << examples.size() << " examples." << endl;
				cout << chrono::duration_cast<chrono::minutes>(chrono::steady_clock::now()-begin).count() << " minutes have passed" << endl;
				
				curNN.train(examples, BATCH_SIZE);
				if (!curNN.save("models/temp2.pt")) {
					cout << "ERROR: Current model did not save correctly to models/temp2.pt" << endl;
				}
			}
		}
		
		curMCTS = MCTS<UTTTNet, UTTTGameState>(curNN, SIMULATIONS);
		prevMCTS = MCTS<UTTTNet, UTTTGameState>(prevNN, SIMULATIONS);
		
		int prevWins = 0, curWins = 0;
		for (int game=0;game<GAMES;game++) {
			cout << "Starting game " << game << endl;
			cout << chrono::duration_cast<chrono::minutes>(chrono::steady_clock::now()-begin).count() << " minutes have passed" << endl;
			UTTTGameState gameState;
			
			int startingPlayer = game % 2; //0 means previous model and 1 means current model
			int player = startingPlayer; //0 means previous model and 1 means current model
			while (gameState.getEnd() == 2) {
				if (DISPLAY_GAMES != 0) {
					displayUTTTBoard(gameState.getBoard());
					
					pair<vector<float>, float> results = prevNN.predict(gameState.getBoard());
					displayUTTTProbs(results.first);
					cout << results.second << endl;
					
					results = curNN.predict(gameState.getBoard());
					displayUTTTProbs(results.first);
					cout << results.second << endl;
				}
				
				vector<float> probs = (player == 0) ? prevMCTS.getMoveProbs(gameState) : curMCTS.getMoveProbs(gameState);
				
				if (DISPLAY_GAMES != 0) {
					displayUTTTProbs(probs);
				}
				
				int move = -1;
				float maxProbs = -1;
				for (int i=0;i<probs.size();i++) {
					if (probs.at(i) > maxProbs) {
						move = i;
						maxProbs = probs.at(i);
					}
				}
				
				gameState = gameState.getChild(move);
				player = 1 - player;
			}
			
			if (gameState.getEnd() == startingPlayer) {
				cout << "Previous model wins! " << endl;
				prevWins++;
			} else if (gameState.getEnd() == 1 - startingPlayer) {
				cout << "Current model wins!" << endl;
				curWins++;
			} else {
				cout << "It's a tie." << endl;
			}
		}
		curMCTS.reset();
		prevMCTS.reset();
		
		cout << "Previous model wins: " << prevWins << endl;
		cout << "Current model wins: " << curWins << endl;
		
		if ((prevWins + curWins == 0) || (float(curWins) / (prevWins + curWins) < 0.55f)) {
			if (curNN.load("models/temp.pt")) {
				cout << "ERROR: Current model did not load correctly from models/temp.pt" << endl;
			}
		} else {
			if (!curNN.save("models/"+to_string(iteration)+".pt")) {
				cout << "ERROR: Current model did not save correctly to models/"+to_string(iteration)+".pt" << endl;
			}
			
			if (!curNN.save("models/best.pt")) {
				cout << "ERROR: Current model did not save correctly to models/best.pt" << endl;
			}
		}
		
		cout << "Iteration " << iteration << " took " << chrono::duration_cast<chrono::minutes>(chrono::steady_clock::now()-begin).count() << " minutes" << endl;
	}
	
	return 0;
}