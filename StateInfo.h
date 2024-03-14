/* Author: Hanuman Chu
 * 
 * Creates StateInfo struct which holds information about a GameState just for use in MCTS
 */
#ifndef STATE_INFO_H
#define STATE_INFO_H

struct StateInfo {
	/**
	 * @brief holds the probabilities of making each move given by a neural network and normalized so that the probabilities still
	 *        add up to one after the probabilities of invalid moves are set to zero
	 */
	std::vector<float> moveProbs = {};
    /**
	 * @brief the number of simulations that went past this game state
	 */
	int simulations = 0;
	/**
	 * @brief the number of times the game state was visited by a parent
	 */
	int visits = 0;
	/**
	 * @brief the total value from all of the simulations that went past this game state
	 */
	float totalValue = 0.0f;
};

#endif
 