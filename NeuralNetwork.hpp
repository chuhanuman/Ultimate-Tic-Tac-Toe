/* Author: Hanuman Chu
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
	std::pair<std::vector<float>, float> predict(const std::vector<float> BOARD);
	
	/**
	 * @brief Trains neural net on given examples using the given batch size
	 * @param EXAMPLES std::vector of tuples each holding a game board, the move probabilities for that game board, and the value of that game board
	 * @param BATCH_SIZE number of examples to include in each batch
	 * @throws invalid_argument if one of the examples has an invalid size for the board or move probabilities
	 */
	void train(const std::vector<std::tuple<std::vector<float>, std::vector<float>, float, int>> EXAMPLES, const int BATCH_SIZE);
	
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
std::pair<std::vector<float>, float> NeuralNetwork<T>::predict(const std::vector<float> BOARD) {
	if (BOARD.size() != mBoardSize) {
		throw std::invalid_argument("Board is not the correct size.");
	}
	
	std::vector<float> board = BOARD;
	torch::Tensor tBoard = torch::from_blob(board.data(), {1, mBoardSize}, torch::TensorOptions(torch::kCPU)).clone();
	
	torch::NoGradGuard no_grad;
	mNet->eval();
	mNet->to(torch::Device(torch::kCPU));
	
	std::vector<torch::Tensor> results = mNet(tBoard);
	results.at(0) = results.at(0).exp();
	std::vector<float> probs = std::vector<float>(results.at(0).data_ptr<float>(), results.at(0).data_ptr<float>() + results.at(0).numel());
	float value = results.at(1).item<float>();
	
	return {probs, value};
}

template<typename T>
void NeuralNetwork<T>::train(const std::vector<std::tuple<std::vector<float>, std::vector<float>, float, int>> EXAMPLES, const int BATCH_SIZE) {
	for (const std::tuple<std::vector<float>, std::vector<float>, float, int> EXAMPLE:EXAMPLES) {
		if (std::get<0>(EXAMPLE).size() != mBoardSize || std::get<1>(EXAMPLE).size() != mBoardSize) {
			throw std::invalid_argument("At least one example was not correctly formatted.");
		}
	}
	
	torch::optim::Adam optimizer(mNet->parameters());
	std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> distribution(0,EXAMPLES.size()-1);
	
	mNet->train();
	mNet->to(torch::Device(torch::kCPU));
	
	int batchCount = EXAMPLES.size() / BATCH_SIZE;
	for (int batch=0;batch<batchCount;batch++) {
		std::vector<float> boards, probs, values;
		
		for (int example=0;example<BATCH_SIZE;example++) {
			int index = distribution(generator);
			boards.insert(boards.end(), std::get<0>(EXAMPLES.at(index)).begin(), std::get<0>(EXAMPLES.at(index)).end());
			probs.insert(probs.end(), std::get<1>(EXAMPLES.at(index)).begin(), std::get<1>(EXAMPLES.at(index)).end());
			values.push_back(std::get<2>(EXAMPLES.at(index)));
		}
		
		torch::Tensor tBoards = torch::from_blob(boards.data(), {BATCH_SIZE, mBoardSize}).clone().to(torch::Device(torch::kCPU));
		torch::Tensor tProbs = torch::from_blob(probs.data(), {BATCH_SIZE, mBoardSize}).clone().to(torch::Device(torch::kCPU));
		torch::Tensor tValues = torch::from_blob(values.data(), {BATCH_SIZE, 1}).clone().to(torch::Device(torch::kCPU));
		
		std::vector<torch::Tensor> results = mNet->forward(tBoards);
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