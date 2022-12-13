/* CSCI 200: Final Project: Ultimate Tic Tac Toe
 *
 * Author: Hanuman Chu
 * Resources used (Office Hours, Tutoring, Other Students, etc & in what capacity):
 *     // list here any outside assistance you used/received while following the
 *     // CS@Mines Collaboration Policy and the Mines Academic Code of Honor
 * 	   Designed By Hugo - youtube.com/watch?v=Kx5CN-V6FvQ - Used to create barebones of UTTTGameWindow class
 *     Surag Nair - web.stanford.edu/~surag/posts/alphazero.html - Used for creating the trainer executable's main function, used to adapt MCTS to be used with neural networks, and used to create the NeuralNetwork and UTTTNet classes and StateInfo struct
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
	vector<float> moveProbs = {};
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
 