/* Author: Hanuman Chu
 * 
 * Creates Ultimate Tic Tac Toe NetImpl struct which implements Net and registers it as a torch module under the name UTTTNet
 */
#ifndef UTTT_NET_H
#define UTTT_NET_H

#include <torch/torch.h>
using namespace torch;

#include <vector>
using namespace std;

struct UTTTNetImpl : public nn::Module {
	nn::Conv2d mConv1, mConv2, mConv3, mConv4;
	nn::BatchNorm2d mBn1, mBn2, mBn3, mBn4;
	nn::Linear mFc1, mFc2, mFc3, mFc4;
	nn::BatchNorm1d mFcBn1, mFcBn2;
	
	/**
	 * @brief Constructor which initializes neural net layers
	 */
	UTTTNetImpl() : 
		mConv1(nn::Conv2dOptions(1, 512, 3).stride(1).padding(1)),
		mConv2(nn::Conv2dOptions(512, 512, 3).stride(1).padding(1)),
		mConv3(nn::Conv2dOptions(512, 512, 3).stride(1)),
		mConv4(nn::Conv2dOptions(512, 512, 3).stride(1)),
		mBn1(512), mBn2(512), mBn3(512), mBn4(512),
		mFc1(512 * 5 * 5, 1024), mFcBn1(1024), mFc2(1024, 512), mFcBn2(512), mFc3(512, 9 * 9), mFc4(512, 1)
	{
		register_module("conv1",mConv1);
		register_module("conv2",mConv2);
		register_module("conv3",mConv3);
		register_module("conv4",mConv4);
		register_module("bn1",mBn1);
		register_module("bn2",mBn2);
		register_module("bn3",mBn3);
		register_module("bn4",mBn4);
		register_module("fc1",mFc1);
		register_module("fcBn1",mFcBn1);
		register_module("fc2",mFc2);
		register_module("fcBn2",mFcBn2);
		register_module("fc3",mFc3);
		register_module("fc4",mFc4);
	}
	
	/**
	 * @brief Takes in batch of UTTT game boards and returns move probabilties and game state value for each game board
	 * @param x tensor which contains game boards
	 * @return vector with 2 entries, the first entry contains lists of probabilities for moves on game boards,
	 *         second entry contains values for the corresponding game boards
	 */
	vector<Tensor> forward(Tensor x) {
		x = x.view({-1, 1, 9, 9}); //batchSize, 1, 9, 9
		
		x = relu(mBn1(mConv1(x))); //batchSize by 512 by 9 by 9
		x = relu(mBn2(mConv2(x))); //batchSize by 512 by 9 by 9
		x = relu(mBn3(mConv3(x))); //batchSize by 512 by 7 by 7
		x = relu(mBn4(mConv4(x))); //batchSize by 512 by 5 by 5
		x = x.view({-1, 512 * 5 * 5}); //batchSize by 512 * 5 * 5
		
		x = dropout(relu(mFcBn1(mFc1(x))), 0.3, is_training()); //batchSize by 1024
		x = dropout(relu(mFcBn2(mFc2(x))), 0.3, is_training()); //batchSize by 512
		
		Tensor probs = mFc3(x); //batchSize by 81
		Tensor value = mFc4(x); //batchSize by 1
		
		return {log_softmax(probs, 1), value.sigmoid()};
	}
};
TORCH_MODULE(UTTTNet);

#endif