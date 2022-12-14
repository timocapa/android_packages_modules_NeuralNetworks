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

#include "InstanceNormalization.h"

#include <cmath>
#include <vector>

#include "OperationResolver.h"
#include "Tracing.h"

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
#include "CpuOperationUtils.h"
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

namespace android {
namespace nn {
namespace instance_normalization {

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
namespace {

template <typename T>
inline bool instanceNormNhwc(const T* inputData, const Shape& inputShape, T gamma, T beta,
                             T epsilon, T* outputData, const Shape& /*outputShape*/) {
    NNTRACE_TRANS("InstanceNormalizationNhwc");
    uint32_t numBatches = getSizeOfDimension(inputShape, 0);
    uint32_t height = getSizeOfDimension(inputShape, 1);
    uint32_t width = getSizeOfDimension(inputShape, 2);
    uint32_t depth = getSizeOfDimension(inputShape, 3);
    for (uint32_t b = 0; b < numBatches; b++) {
        for (uint32_t d = 0; d < depth; d++) {
            uint32_t indexBase = b * height * width * depth + d;
            T mean = 0, sigma = 0;

            // Compute the mean of a single layer.
            for (uint32_t h = 0; h < height; h++) {
                for (uint32_t w = 0; w < width; w++) {
                    T val = inputData[indexBase + (h * width + w) * depth];
                    mean += val;
                }
            }
            mean /= static_cast<T>(height * width);

            // Compute the standard deviation (sigma) of a single layer.
            for (uint32_t h = 0; h < height; h++) {
                for (uint32_t w = 0; w < width; w++) {
                    T val = inputData[indexBase + (h * width + w) * depth] - mean;
                    sigma += val * val;
                }
            }
            sigma = std::sqrt(static_cast<float>(sigma / static_cast<T>(height * width)) + epsilon);

            // Apply instance normalization.
            for (uint32_t h = 0; h < height; h++) {
                for (uint32_t w = 0; w < width; w++) {
                    uint32_t ind = indexBase + (h * width + w) * depth;
                    outputData[ind] = (inputData[ind] - mean) * gamma / sigma + beta;
                }
            }
        }
    }
    return true;
}

template <typename T>
inline bool instanceNorm(const T* inputData, const Shape& inputShape, T gamma, T beta, T epsilon,
                         bool useNchw, T* outputData, const Shape& outputShape) {
    InputWithLayout<T> input(useNchw);
    OutputWithLayout<T> output(useNchw);
    NN_RET_CHECK(input.initialize(inputData, inputShape));
    NN_RET_CHECK(output.initialize(outputData, outputShape));
    NN_RET_CHECK(instanceNormNhwc(input.getNhwcBuffer(), input.getNhwcShape(), gamma, beta, epsilon,
                                  output.getNhwcBuffer(), output.getNhwcShape()));
    NN_RET_CHECK(output.commit());
    return true;
}

}  // namespace

bool prepare(IOperationExecutionContext* context) {
    Shape input = context->getInputShape(kInputTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(input), 4u);
    return context->setOutputShape(kOutputTensor, input);
}

bool execute(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return instanceNorm(context->getInputBuffer<_Float16>(kInputTensor),
                                context->getInputShape(kInputTensor),
                                context->getInputValue<_Float16>(kGammaScalar),
                                context->getInputValue<_Float16>(kBetaScalar),
                                context->getInputValue<_Float16>(kEpsilonScalar),
                                context->getInputValue<bool>(kLayoutScalar),
                                context->getOutputBuffer<_Float16>(kOutputTensor),
                                context->getOutputShape(kOutputTensor));
        case OperandType::TENSOR_FLOAT32:
            return instanceNorm(context->getInputBuffer<float>(kInputTensor),
                                context->getInputShape(kInputTensor),
                                context->getInputValue<float>(kGammaScalar),
                                context->getInputValue<float>(kBetaScalar),
                                context->getInputValue<float>(kEpsilonScalar),
                                context->getInputValue<bool>(kLayoutScalar),
                                context->getOutputBuffer<float>(kOutputTensor),
                                context->getOutputShape(kOutputTensor));
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation " << kOperationName;
    }
}
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

}  // namespace instance_normalization

NN_REGISTER_OPERATION_DEFAULT_VALIDATION(INSTANCE_NORMALIZATION, instance_normalization::prepare,
                                         instance_normalization::execute);

}  // namespace nn
}  // namespace android
