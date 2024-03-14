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
#include <map>
#include <fstream>
#include <chrono>
#include <random>
#include <vector>

/**
 * @brief Displays move probabilties in an easy to read format
 * @param probs move probabilities to display
 */
void displayUTTTProbs(std::vector<float> probs) {
	int row = 0, col = 0;
	for (float chance:probs) {
		if (chance < 0.1f) {
			std::cout << "0" << (int)(chance * 100) << " ";
		} else if (chance == 1.0f) {
			std::cout << "!! ";
		} else {
			std::cout << (int)(chance * 100) << " ";
		}
		
		if (col == 8) {
			col = -1;
			if (row % 3 == 2) {
				std::cout << '\n';
			}
			std::cout << '\n';
			row++;
		} else if (col % 3 == 2) {
			std::cout << " ";
		}
		col++;
	}
}

/**
 * @brief Displays game board in an easy to read format
 * @param board game board to display
 */
void displayUTTTBoard(std::vector<float> board) {
	for (int i=0;i<board.size();i++) {
		std::cout << (int)board[i] << " ";
		if (i % 9 == 8) {
			std::cout << '\n';
			if (i % 27 == 26) {
				std::cout << '\n';
			}
		} else if (i % 3 == 2) {
			std::cout << " ";
		}
	}
}

int main(int argc, char* argv[]) {
	NeuralNetwork<UTTTNet> curNN(81), prevNN(81);
	if (argc >= 2) {
		if (!curNN.load(argv[1])) {
			std::cout << "ERROR: Starting current model did not load correctly from " << argv[1] << '\n' << std::flush;
		}
		if (argc >= 3) {
			if (!prevNN.load(argv[2])) {
				std::cout << "ERROR: Starting previous model did not load correctly from " << argv[2] << '\n' << std::flush;
			}
		}
	} else {
		std::cout << "WARNING: No model was passed." << '\n' << std::flush;
	}
	
	std::ifstream fin("config.txt");
	
	if (fin.fail()) {
		std::cout << "FATAL: Config file did not load correctly" << '\n';
		return 1;
	}
	
	std::vector<int> config;
	int iTemp;
	for (int i=0;i<10;i++) {
		fin >> iTemp;
		
		if (fin.fail()) {
			std::cout << "FATAL: Config file did not load correctly" << '\n';
			return 1;
		}
		
		config.push_back(iTemp);
		fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	fin.close();
	
	const int ITERATIONS = config.at(0), EPISODES = config.at(1), SIMULATIONS = config.at(2), GAMES = config.at(3), EXPLORATION_TURNS = config.at(4), BATCH_SIZE = config.at(5), EPOCHS = config.at(8), LOAD_EXAMPLES = config.at(7), SKIP_TRAINING = config.at(8), DISPLAY_GAMES = config.at(9);
	
	MCTS<UTTTNet, UTTTGameState> curMCTS = MCTS<UTTTNet, UTTTGameState>(curNN, SIMULATIONS);
	MCTS<UTTTNet, UTTTGameState> prevMCTS = MCTS<UTTTNet, UTTTGameState>(prevNN, SIMULATIONS);
	std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	for (int iteration=0;iteration<ITERATIONS;iteration++) {
		std::cout << "Starting iteration " << iteration << '\n';
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		
		std::vector<std::tuple<std::vector<float>, std::vector<float>, float, int>> examples; //examples in format {board, probs, total value, number of episodes}
		if (LOAD_EXAMPLES == 0 || iteration != 0) {
			std::map<std::vector<float>, std::tuple<std::vector<float>, float, int>> preExamples; //std::maps game board to {probs, total value, number of episodes}
			
			for (int episode=0;episode<EPISODES;episode++) {
				std::cout << "Starting episode " << episode << '\n';
				std::cout << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes have passed" << '\n' << std::flush;
				UTTTGameState gameState;
				std::vector<std::vector<float>> curKeys;
				
				int turns = 0;
				std::vector<float> probs;
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
					
					for (std::pair<std::vector<float>,std::vector<float>> symmetry:gameState.getSymmetries(probs)) {
						if (find(curKeys.begin(), curKeys.end(), symmetry.first) == curKeys.end()) {
							curKeys.push_back(symmetry.first);
							
							if (preExamples.find(symmetry.first) == preExamples.end()) {
								preExamples.emplace(symmetry.first, make_tuple(symmetry.second, 0.0f, 0));
							}
						}
					}
					
					std::discrete_distribution<int> distribution(probs.begin(), probs.end());
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
				
				for (std::vector<float> key:curKeys) {
					std::get<1>(preExamples.at(key)) += result;
					std::get<2>(preExamples.at(key))++;
				}
				
				curMCTS.reset();
				
				if ((episode + 1) % (EPISODES / 10) == 0) {
					std::ofstream fout("temp.ex");
					for (std::map<std::vector<float>, std::tuple<std::vector<float>, float, int>>::iterator i=preExamples.begin();i!=preExamples.end();i++) {
						for (int j=0;j<81;j++) {
							fout << i->first.at(j) << " ";
							fout << std::get<0>(i->second).at(j) << " ";
						}
						fout << std::get<1>(i->second) << " " << std::get<2>(i->second) << '\n';
					}
					
					fout.close();
				}
			}
			
			std::ofstream fout("temp.ex");
			for (std::map<std::vector<float>, std::tuple<std::vector<float>, float, int>>::iterator i=preExamples.begin();i!=preExamples.end();i++) {
				examples.push_back(make_tuple(i->first, std::get<0>(i->second), std::get<1>(i->second), std::get<2>(i->second)));
				
				for (int j=0;j<81;j++) {
					fout << i->first.at(j) << " ";
					fout << std::get<0>(i->second).at(j) << " ";
				}
				fout << std::get<1>(i->second) << " " << std::get<2>(i->second) << '\n';
			}
			
			fout.close();
		}
		
		if (SKIP_TRAINING == 0 || iteration != 0) {
			if (!curNN.save("models/temp.pt")) {
				std::cout << "ERROR: Current model did not save correctly to models/temp.pt" << '\n' << std::flush;
			}
			
			if (!prevNN.load("models/temp.pt")) {
				std::cout << "ERROR: Previous model did not load correctly from models/temp.pt" << '\n' << std::flush;
			}
			
			for (int epoch=0;epoch<EPOCHS * ((LOAD_EXAMPLES > 0 && iteration == 0) ? LOAD_EXAMPLES : 1);epoch++) {
				if (LOAD_EXAMPLES > 0 && iteration == 0) {
					std::uniform_int_distribution<int> distribution(1,LOAD_EXAMPLES);
					examples = {};
					
					std::ifstream fin("examples/temp" + to_string(distribution(generator)) + ".ex");
					
					float fTemp;
					while (!fin.eof()) {
						std::tuple<std::vector<float>, std::vector<float>, float, float> example;
						
						for (int i=0;i<81;i++) {
							fin >> fTemp;
							std::get<0>(example).push_back(fTemp);
							fin >> fTemp;
							std::get<1>(example).push_back(fTemp);
						}
						fin >> std::get<2>(example);
						
						if (fin.fail()) {
							break;
						}
						
						examples.push_back(example);
					}
					
					fin.close();
				}
				
				std::cout << "Training with " << examples.size() << " examples." << '\n';
				std::cout << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes have passed" << '\n' << std::flush;
				
				curNN.train(examples, BATCH_SIZE);
				if (!curNN.save("models/temp2.pt")) {
					std::cout << "ERROR: Current model did not save correctly to models/temp2.pt" << '\n' << std::flush;
				}
			}
		}
		
		curMCTS = MCTS<UTTTNet, UTTTGameState>(curNN, SIMULATIONS);
		prevMCTS = MCTS<UTTTNet, UTTTGameState>(prevNN, SIMULATIONS);
		
		int prevWins = 0, curWins = 0;
		for (int game=0;game<GAMES;game++) {
			std::cout << "Starting game " << game << '\n';
			std::cout << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes have passed" << '\n' << std::flush;
			UTTTGameState gameState;
			
			int startingPlayer = game % 2; //0 means previous model and 1 means current model
			int player = startingPlayer; //0 means previous model and 1 means current model
			while (gameState.getEnd() == 2) {
				if (DISPLAY_GAMES != 0) {
					displayUTTTBoard(gameState.getBoard());
					
					std::pair<std::vector<float>, float> results = prevNN.predict(gameState.getBoard());
					displayUTTTProbs(results.first);
					std::cout << results.second << '\n';
					
					results = curNN.predict(gameState.getBoard());
					displayUTTTProbs(results.first);
					std::cout << results.second << '\n';
				}
				
				std::vector<float> probs = (player == 0) ? prevMCTS.getMoveProbs(gameState) : curMCTS.getMoveProbs(gameState);
				
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
				std::cout << "Previous model wins! " << '\n' << std::flush;
				prevWins++;
			} else if (gameState.getEnd() == 1 - startingPlayer) {
				std::cout << "Current model wins!" << '\n' << std::flush;
				curWins++;
			} else {
				std::cout << "It's a tie." << '\n' << std::flush;
			}
		}
		curMCTS.reset();
		prevMCTS.reset();
		
		std::cout << "Previous model wins: " << prevWins << '\n';
		std::cout << "Current model wins: " << curWins << '\n' << std::flush;
		
		if ((prevWins + curWins == 0) || (float(curWins) / (prevWins + curWins) < 0.55f)) {
			if (!curNN.load("models/temp.pt")) {
				std::cout << "ERROR: Current model did not load correctly from models/temp.pt" << '\n' << std::flush;
			}
		} else {
			if (!curNN.save("models/"+to_string(iteration)+".pt")) {
				std::cout << "ERROR: Current model did not save correctly to models/"+to_string(iteration)+".pt" << '\n' << std::flush;
			}
			
			if (!curNN.save("models/best.pt")) {
				std::cout << "ERROR: Current model did not save correctly to models/best.pt" << '\n' << std::flush;
			}
		}
		
		std::cout << "Iteration " << iteration << " took " << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes" << '\n' << std::flush;
	}
	
	return 0;
}