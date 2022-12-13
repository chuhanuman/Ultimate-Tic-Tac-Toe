/* CSCI 200: Final Project: Ultimate Tic Tac Toe
 *
 * Author: Hanuman Chu
 * Resources used (Office Hours, Tutoring, Other Students, etc & in what capacity):
 *     // list here any outside assistance you used/received while following the
 *     // CS@Mines Collaboration Policy and the Mines Academic Code of Honor
 * 	   Designed By Hugo - youtube.com/watch?v=Kx5CN-V6FvQ - Used to create barebones of UTTTGameWindow class
 *     Surag Nair - web.stanford.edu/~surag/posts/alphazero.html - Used for creating the trainer executable's main function, used to adapt MCTS to be used with neural networks, and used to create the NeuralNetwork and UTTTNet classes and StateInfo struct
 * 
 * Creates templated Neural Network class which has loading and saving functionality
 */
#ifndef NEURAL_NETWORK_HPP
#define NEURAL_NETWORK_HPP

#include <torch/torch.h>

#include <string>
#include <random>
#include <stdexcept>
#include <vector>
using namespace std;

template<typename T>
class NeuralNetwork {
public:
	/**
	 * @brief Constructor which sets the board size to the given size with a minimum of one
	 * @param BOARD_SIZE size of the game boards the neural network will accept
	 */
	NeuralNetwork(unsigned const int BOARD_SIZE);
	
	/**
	 * @brief Runs given board through neural net and returns the results
	 * @param BOARD game board to run through neural net  
	 * @return pair with the first element being the move probabilities of the given board and the second element being the value of the given board
	 * @throws invalid_argument if the board is not of the correct size
	 */
	pair<vector<float>, float> predict(const vector<float> BOARD);
	
	/**
	 * @brief Trains neural net on given examples using the given batch size
	 * @param EXAMPLES vector of tuples each holding a game board, the move probabilities for that game board, and the value of that game board
	 * @param BATCH_SIZE number of examples to include in each batch
	 * @throws invalid_argument if one of the examples has an invalid size for the board or move probabilities
	 */
	void train(const vector<tuple<vector<float>, vector<float>, float>> EXAMPLES, const int BATCH_SIZE);
	
	/**
	 * @brief Loads neural net from file path and returns whether it was successful
	 * @param FILE_PATH file path to load neural net from 
	 * @return whether the neural net loaded successfully 
	 */
	bool load(const string FILE_PATH);
	
	/**
	 * @brief Saves neural net to file path and returns whether it was successful
	 * @param FILE_PATH file path to save neural net to 
	 * @return whether the neural net saved successfully 
	 */
	bool save(const string FILE_PATH) const;
private:
	/**
	 * @brief neural net to run boards through
	 */
	T mNet;
	/**
	 * @brief size of the boards to accept
	 */
	unsigned int mBoardSize;
};

template<typename T>
NeuralNetwork<T>::NeuralNetwork(unsigned const int BOARD_SIZE) {
	if (BOARD_SIZE < 1) {
		mBoardSize = 1;
	} else {
		mBoardSize = BOARD_SIZE;
	}
}

template<typename T>
pair<vector<float>, float> NeuralNetwork<T>::predict(const vector<float> BOARD) {
	if (BOARD.size() != mBoardSize) {
		throw invalid_argument("Board is not the correct size.");
	}
	
	vector<float> board = BOARD;
	torch::Tensor tBoard = torch::from_blob(board.data(), {1, mBoardSize}, torch::TensorOptions(torch::kCPU)).clone();
	
	torch::NoGradGuard no_grad;
	mNet->eval();
	mNet->to(torch::Device(torch::kCPU));
	
	vector<torch::Tensor> results = mNet(tBoard);
	results.at(0) = results.at(0).exp();
	vector<float> probs = vector<float>(results.at(0).data_ptr<float>(), results.at(0).data_ptr<float>() + results.at(0).numel());
	float value = results.at(1).item<float>();
	
	return {probs, value};
}

template<typename T>
void NeuralNetwork<T>::train(const vector<tuple<vector<float>, vector<float>, float>> EXAMPLES, const int BATCH_SIZE) {
	for (const tuple<vector<float>, vector<float>, float> EXAMPLE:EXAMPLES) {
		if (get<0>(EXAMPLE).size() != mBoardSize || get<1>(EXAMPLE).size() != mBoardSize) {
			throw invalid_argument("At least one example was not correctly formatted.");
		}
	}
	
	torch::optim::Adam optimizer(mNet->parameters());
	default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
	uniform_int_distribution<int> distribution(0,EXAMPLES.size()-1);
	
	mNet->train();
	mNet->to(torch::Device(torch::kCPU));
	
	int batchCount = EXAMPLES.size() / BATCH_SIZE;
	for (int batch=0;batch<batchCount;batch++) {
		vector<float> boards, probs, values;
		
		for (int example=0;example<BATCH_SIZE;example++) {
			int index = distribution(generator);
			boards.insert(boards.end(), get<0>(EXAMPLES.at(index)).begin(), get<0>(EXAMPLES.at(index)).end());
			probs.insert(probs.end(), get<1>(EXAMPLES.at(index)).begin(), get<1>(EXAMPLES.at(index)).end());
			values.push_back(get<2>(EXAMPLES.at(index)));
		}
		
		torch::Tensor tBoards = torch::from_blob(boards.data(), {BATCH_SIZE, mBoardSize}).clone().to(torch::Device(torch::kCPU));
		torch::Tensor tProbs = torch::from_blob(probs.data(), {BATCH_SIZE, mBoardSize}).clone().to(torch::Device(torch::kCPU));
		torch::Tensor tValues = torch::from_blob(values.data(), {BATCH_SIZE, 1}).clone().to(torch::Device(torch::kCPU));
		
		vector<torch::Tensor> results = mNet->forward(tBoards);
		torch::Tensor probsLoss = -1 * torch::sum(tProbs * results.at(0)) / BATCH_SIZE;
		torch::Tensor valuesLoss = torch::sum(torch::pow(tValues - results.at(1).view(-1), 2)) / BATCH_SIZE;
		torch::Tensor totalLoss = probsLoss + valuesLoss;
		
		optimizer.zero_grad();
		totalLoss.backward();
		optimizer.step();
	}
}

template<typename T>
bool NeuralNetwork<T>::load(const string FILE_PATH) {
	try {
		torch::load(mNet, FILE_PATH);
	} catch (...) {
		return false;
	}
	
	return true;
}

template<typename T>
bool NeuralNetwork<T>::save(const string FILE_PATH) const {
	try {
		torch::save(mNet, FILE_PATH);
	} catch (...) {
		return false;
	}
	
	return true;
}

#endif