/* Author: Hanuman Chu
 *
 * Creates templated MCTS class which uses a version of MCTS modified to work with neural networks
 */
#ifndef MCTS_HPP
#define MCTS_HPP

#include "NeuralNetwork.hpp"
#include "StateInfo.h"

#include <vector>
#include <map>
using namespace std;

const float EXPLORATION_PARAMETER = 1;

template<typename T, typename U>
class MCTS {
public:
	/**
	 * @brief Constructs a new MCTS object with the given neural network and number of simulations with a minimum of 1
	 * @param NN neural network used to predict probabilities and value of game states
	 * @param SIMULATIONS number of simulations to run each time
	 */
	MCTS(NeuralNetwork<T> NN, const unsigned int SIMULATIONS);
	
	/**
	 * @brief Runs simulations on given game state and returns move probabilities corresponding to the number of times each move was visited
	 * @param BASE_GAME_STATE game state to start simulations on
	 * @return list of probabilities for each move
	 */
	vector<float> getMoveProbs(const U BASE_GAME_STATE);
	
	/**
	 * @brief Runs simulations on given game state and returns the best move which is the move with the most visits
	 * @param BASE_GAME_STATE game state to start simulations on
	 * @return move list with best move corresponding to a value of one and other moves corresponding to a value of zero
	 */
	vector<float> getBestMove(const U BASE_GAME_STATE);
	
	/**
	 * @brief Sets the number of simulations with a minimum of 1
	 * @param SIMULATIONS number of simulations to run each time
	 */
	void setSimulations(const unsigned int SIMULATIONS);
	
	/**
	 * @brief Resets MCTS tree
	 */
	void reset();
private:
	/**
	 * @brief the neural network used to predict the value and move probabilities of game boards
	 */
	NeuralNetwork<T> mNN;
	/**
	 * @brief the number of simulations to perform each time mMCTS is run
	 */
	unsigned int mSimulations;
	/**
	 * @brief maps game state keys to their state information
	 */
	map<string, StateInfo> mStateInfos;
	
	/**
	 * @brief Recursively looks for unexplored game state using game state's selection score, simulates it, then updates values based on
	 * @brief the simulation result
	 * @param POTENTIAL_LEAF game state to simulate
	 * @return final value of simulation
	 */
	float mSimulate(const U POTENTIAL_LEAF);
	
	/**
	 * @brief Runs simulations on given game state
	 * @param BASE_GAME_STATE game state to start simulations on
	 */
	void mMCTS(const U BASE_GAME_STATE);
};

template<typename T, typename U>
MCTS<T, U>::MCTS(NeuralNetwork<T> NN, const unsigned int SIMULATIONS) : mNN(NN) {
	if (SIMULATIONS < 1) {
		mSimulations = 1;
	} else {
		mSimulations = SIMULATIONS;
	}
}

template<typename T, typename U>
vector<float> MCTS<T, U>::getMoveProbs(const U BASE_GAME_STATE) {
	mMCTS(BASE_GAME_STATE);
	
	float totalSimulations = 1.0f;
	if (mStateInfos.find(BASE_GAME_STATE.getKey()) != mStateInfos.end()) {
		totalSimulations = mStateInfos.at(BASE_GAME_STATE.getKey()).simulations;
	}
	
	vector<float> newMoveProbs;
	for (int move = 0; move < 81; move++) {
		if (BASE_GAME_STATE.isValid(move) && mStateInfos.find(BASE_GAME_STATE.getChild(move).getKey()) != mStateInfos.end()) {
			newMoveProbs.push_back(mStateInfos.at(BASE_GAME_STATE.getChild(move).getKey()).visits / totalSimulations);
		} else {
			newMoveProbs.push_back(0.0f);
		}
	}
	
	return newMoveProbs;
}

template<typename T, typename U>
vector<float> MCTS<T, U>::getBestMove(const U BASE_GAME_STATE) {
	mMCTS(BASE_GAME_STATE);
	
	int mostVisited = -1;
	int mostVisits = -1;
	for (int move:BASE_GAME_STATE.getValidMoves()) {
		if (mStateInfos.find(BASE_GAME_STATE.getChild(move).getKey()) == mStateInfos.end()) {
			continue;
		}
		
		int visits = mStateInfos.at(BASE_GAME_STATE.getChild(move).getKey()).visits;
		if (visits > mostVisits) {
			mostVisits = visits;
			mostVisited = move;
		}
	}
	
	vector<float> newMoveProbs;
	for (int move = 0; move < 81; move++) {
		newMoveProbs.push_back(move == mostVisited ? 1.0f : 0.0f);
	}
	
	return newMoveProbs;
}

template<typename T, typename U>
void MCTS<T, U>::setSimulations(const unsigned int SIMULATIONS) {
    if (SIMULATIONS < 1) {
		mSimulations = 1;
	} else {
		mSimulations = SIMULATIONS;
	}
}

template<typename T, typename U>
void MCTS<T, U>::reset() {
    mStateInfos.clear();
}


template<typename T, typename U>
float MCTS<T, U>::mSimulate(const U POTENTIAL_LEAF) {
	if (POTENTIAL_LEAF.getEnd() != 2) {
		if (POTENTIAL_LEAF.getEnd() == 3) {
			return 0.5f;
		} else {
			return POTENTIAL_LEAF.getEnd();
		}
	}
	
	map<string, StateInfo>::iterator leafStateInfoIter = mStateInfos.find(POTENTIAL_LEAF.getKey());
	
	if (leafStateInfoIter == mStateInfos.end()) {
		//Evaluates leaf
		pair<vector<float>, float> results = mNN.predict(POTENTIAL_LEAF.getBoard());
		
		float total = 0.0f;
		for (int move:POTENTIAL_LEAF.getValidMoves()) {
			total += results.first.at(move);
		}
		
		vector<float> moveProbs;
		for (int move = 0; move < 81; move++) {
			moveProbs.push_back(POTENTIAL_LEAF.isValid(move) ? (results.first.at(move) / total) : 0.0f);
		}
		
		StateInfo stateInfo;
		stateInfo.moveProbs = moveProbs;
		mStateInfos.emplace(POTENTIAL_LEAF.getKey(), stateInfo);
		
		return results.second;
	}
	
	//Selects child to explore
	float bestSelectionScore = -1.0f;
	int bestSelection = -1;
	for (int move:POTENTIAL_LEAF.getValidMoves()) {
		float selectionScore;
		
		map<string, StateInfo>::iterator childStateInfoIter = mStateInfos.find(POTENTIAL_LEAF.getChild(move).getKey());
		
		if (childStateInfoIter != mStateInfos.end()) {
			float childValue = childStateInfoIter->second.totalValue / childStateInfoIter->second.visits;
			if (POTENTIAL_LEAF.getNextPlayer() == 0) {
				childValue = 1 - childValue;
			}
			
			selectionScore = childValue + EXPLORATION_PARAMETER * leafStateInfoIter->second.moveProbs.at(move) * sqrt((float)leafStateInfoIter->second.simulations) / (childStateInfoIter->second.visits + 1);
		} else {
			selectionScore = 0.5f + EXPLORATION_PARAMETER * leafStateInfoIter->second.moveProbs.at(move) * sqrt((float)leafStateInfoIter->second.simulations + 0.00000001f);
		}
		
		if (selectionScore > bestSelectionScore) {
			bestSelectionScore = selectionScore;
			bestSelection = move;
		}
	}
	
	const U CHILD = POTENTIAL_LEAF.getChild(bestSelection);
	
	float value = mSimulate(CHILD);
	
	map<string, StateInfo>::iterator childStateInfoIter = mStateInfos.find(CHILD.getKey());
	
	//Updates values
	if (childStateInfoIter != mStateInfos.end()) {
		childStateInfoIter->second.totalValue += value;
		childStateInfoIter->second.visits++;
	} else {
		StateInfo stateInfo;
		stateInfo.totalValue = value;
		stateInfo.visits++;
		mStateInfos.emplace(CHILD.getKey(), stateInfo);
	}
	leafStateInfoIter->second.simulations++;
	
	return value;
}

template<typename T, typename U>
void MCTS<T, U>::mMCTS(const U BASE_GAME_STATE) {
    for (unsigned int i=0;i<mSimulations;i++) {
        mSimulate(BASE_GAME_STATE);
    }
}

#endif