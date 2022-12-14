/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Operations"

#include "UnidirectionalSequenceRNN.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "OperationResolver.h"
#include "RNN.h"
#include "nnapi/TypeUtils.h"

namespace android {
namespace nn {
namespace unidirectional_sequence_rnn {

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
namespace {

template <typename T>
void transposeFirstTwoDims(const T* input, const Shape& inputShape, T* output) {
    const uint32_t firstDimSize = getSizeOfDimension(inputShape, 0);
    const uint32_t secondDimSize = getSizeOfDimension(inputShape, 1);
    const uint32_t inputSize = getSizeOfDimension(inputShape, 2);
    for (uint32_t f = 0; f < firstDimSize; ++f) {
        for (uint32_t s = 0; s < secondDimSize; ++s) {
            for (uint32_t i = 0; i < inputSize; ++i) {
                const uint32_t inputIndex = f * secondDimSize * inputSize + s * inputSize + i;
                const uint32_t outputIndex = s * firstDimSize * inputSize + f * inputSize + i;
                output[outputIndex] = input[inputIndex];
            }
        }
    }
}

template <typename T>
bool executeTyped(IOperationExecutionContext* context) {
    const T* input = context->getInputBuffer<T>(kInputTensor);
    Shape inputShape = context->getInputShape(kInputTensor);
    const T* weights = context->getInputBuffer<T>(kWeightsTensor);
    Shape weightsShape = context->getInputShape(kWeightsTensor);
    const T* recurrentWeights = context->getInputBuffer<T>(kRecurrentWeightsTensor);
    Shape recurrentWeightsShape = context->getInputShape(kRecurrentWeightsTensor);
    const T* bias = context->getInputBuffer<T>(kBiasTensor);
    const T* hiddenState = context->getInputBuffer<T>(kHiddenStateTensor);
    int32_t activation = context->getInputValue<int32_t>(kActivationParam);

    T* output = context->getOutputBuffer<T>(kOutputTensor);
    Shape outputShape = context->getOutputShape(kOutputTensor);

    int32_t timeMajor = context->getInputValue<int32_t>(kTimeMajorParam);
    // If the input tensors are not in time major format, we transpose the first
    // two dimensions, and set input and output pointers to temporary vectors
    // which are transposed back after the RNN is applied.
    std::vector<T> inputTransposed;
    std::vector<T> outputTransposed;
    if (!timeMajor) {
        // Convert input and output to time major format.
        inputTransposed.resize(getNumberOfElements(inputShape));
        outputTransposed.resize(getNumberOfElements(outputShape));
        transposeFirstTwoDims(input, inputShape, inputTransposed.data());
        input = inputTransposed.data();
        output = outputTransposed.data();
        std::swap(inputShape.dimensions[0], inputShape.dimensions[1]);
        std::swap(outputShape.dimensions[0], outputShape.dimensions[1]);
    }

    const uint32_t maxTime = getSizeOfDimension(inputShape, 0);
    const uint32_t batchSize = getSizeOfDimension(inputShape, 1);
    const uint32_t inputSize = getSizeOfDimension(inputShape, 2);
    const uint32_t numUnits = getSizeOfDimension(weightsShape, 0);

    // A shape at a fixed step (removed time dimension).
    Shape fixedTimeInputShape = inputShape;
    fixedTimeInputShape.dimensions.resize(2);
    fixedTimeInputShape.dimensions[0] = inputShape.dimensions[1];
    fixedTimeInputShape.dimensions[1] = inputShape.dimensions[2];

    for (uint32_t i = 0; i < maxTime; ++i) {
        RNN::RNNStep<T>(input, fixedTimeInputShape, hiddenState, bias, weights, weightsShape,
                        recurrentWeights, recurrentWeightsShape, activation, output);
        input += batchSize * inputSize;
        hiddenState = output;
        output += batchSize * numUnits;
    }

    if (!timeMajor) {
        transposeFirstTwoDims(outputTransposed.data(), outputShape,
                              context->getOutputBuffer<T>(kOutputTensor));
    }

    if (context->getNumOutputs() == kNumOutputsWithState) {
        // We checked that the state output is not omitted during preparation.
        T* stateOutput = context->getOutputBuffer<T>(kStateOutputTensor);
        std::copy(hiddenState, hiddenState + batchSize * numUnits, stateOutput);
    }
    return true;
}

}  // namespace

bool prepare(IOperationExecutionContext* context) {
    Shape input = context->getInputShape(kInputTensor);
    Shape weights = context->getInputShape(kWeightsTensor);
    Shape recurrentWeights = context->getInputShape(kRecurrentWeightsTensor);
    Shape bias = context->getInputShape(kBiasTensor);
    Shape hiddenState = context->getInputShape(kHiddenStateTensor);

    int32_t timeMajor = context->getInputValue<int32_t>(kTimeMajorParam);
    NN_RET_CHECK(timeMajor == 0 || timeMajor == 1);
    const uint32_t batchSize =
            timeMajor ? getSizeOfDimension(input, 1) : getSizeOfDimension(input, 0);
    const uint32_t maxTime =
            timeMajor ? getSizeOfDimension(input, 0) : getSizeOfDimension(input, 1);
    const uint32_t numUnits = getSizeOfDimension(weights, 0);
    const uint32_t inputSize = getSizeOfDimension(input, 2);

    NN_RET_CHECK_EQ(getNumberOfDimensions(input), 3u);
    NN_RET_CHECK_EQ(getNumberOfDimensions(weights), 2u);
    NN_RET_CHECK_EQ(getNumberOfDimensions(recurrentWeights), 2u);
    NN_RET_CHECK_EQ(getNumberOfDimensions(bias), 1u);
    NN_RET_CHECK_EQ(getNumberOfDimensions(hiddenState), 2u);

    NN_RET_CHECK_EQ(inputSize, getSizeOfDimension(weights, 1));
    NN_RET_CHECK_EQ(numUnits, getSizeOfDimension(bias, 0));
    NN_RET_CHECK_EQ(numUnits, getSizeOfDimension(recurrentWeights, 0));
    NN_RET_CHECK_EQ(numUnits, getSizeOfDimension(recurrentWeights, 1));
    NN_RET_CHECK_EQ(batchSize, getSizeOfDimension(hiddenState, 0));
    NN_RET_CHECK_EQ(numUnits, getSizeOfDimension(hiddenState, 1));

    Shape output = context->getOutputShape(kOutputTensor);
    output.dimensions.resize(3);
    output.dimensions[0] = timeMajor ? maxTime : batchSize;
    output.dimensions[1] = timeMajor ? batchSize : maxTime;
    output.dimensions[2] = numUnits;

    if (context->getNumOutputs() == kNumOutputsWithState) {
        NN_RET_CHECK(!context->isOmittedOutput(kStateOutputTensor));
        Shape outputStateShape = context->getInputShape(kHiddenStateTensor);
        outputStateShape.dimensions.resize(2);
        outputStateShape.dimensions[0] = batchSize;
        outputStateShape.dimensions[1] = numUnits;
        NN_RET_CHECK(context->setOutputShape(kStateOutputTensor, outputStateShape));
    }

    return context->setOutputShape(kOutputTensor, output);
}

bool execute(IOperationExecutionContext* context) {
    if (context->getInputType(kInputTensor) == OperandType::TENSOR_FLOAT16) {
        executeTyped<_Float16>(context);
    } else {
        executeTyped<float>(context);
    }
    return true;
}
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

}  // namespace unidirectional_sequence_rnn

NN_REGISTER_OPERATION_DEFAULT_VALIDATION(UNIDIRECTIONAL_SEQUENCE_RNN,
                                         unidirectional_sequence_rnn::prepare,
                                         unidirectional_sequence_rnn::execute);

}  // namespace nn
}  // namespace android
