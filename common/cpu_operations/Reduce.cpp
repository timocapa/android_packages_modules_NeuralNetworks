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

#include "Reduce.h"

#include <algorithm>
#include <limits>
#include <vector>

#include "OperationResolver.h"
#include "OperationsExecutionUtils.h"
#include "Tracing.h"

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wsign-compare"
#include <tensorflow/lite/kernels/internal/reference/reference_ops.h>
#pragma clang diagnostic pop
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

namespace android {
namespace nn {
namespace reduce {

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
namespace {

template <typename T>
inline bool compute(IOperationExecutionContext* context, T init, T func(T, T)) {
    const Shape inputShape = context->getInputShape(kInputTensor);
    const Shape axesShape = context->getInputShape(kInputAxes);
    const Shape outputShape = context->getOutputShape(kOutputTensor);
    const uint32_t inputRank = getNumberOfDimensions(inputShape);
    const uint32_t numAxes = getNumberOfElements(axesShape);
    std::vector<int> tempIndex(inputShape.dimensions.size());
    std::vector<int> tempAxes(numAxes);
    return tflite::reference_ops::ReduceGeneric<T>(
            context->getInputBuffer<T>(kInputTensor),
            reinterpret_cast<const int32_t*>(inputShape.dimensions.data()), inputRank,
            context->getOutputBuffer<T>(kOutputTensor),
            reinterpret_cast<const int32_t*>(outputShape.dimensions.data()),
            outputShape.dimensions.size(), context->getInputBuffer<int32_t>(kInputAxes), numAxes,
            context->getInputValue<bool8>(kInputKeepDims), tempIndex.data(), tempAxes.data(), init,
            func);
}

}  // namespace

bool prepare(IOperationExecutionContext* context) {
    Shape inputShape = context->getInputShape(kInputTensor);
    const uint32_t inputRank = getNumberOfDimensions(inputShape);
    NN_RET_CHECK_LE(inputRank, 4u);

    std::vector<bool> shouldReduce(inputRank);
    const int32_t* axes = context->getInputBuffer<int32_t>(kInputAxes);
    Shape axesShape = context->getInputShape(kInputAxes);
    NN_RET_CHECK_EQ(getNumberOfDimensions(axesShape), 1u);
    const uint32_t numAxes = getNumberOfElements(axesShape);
    for (uint32_t i = 0; i < numAxes; ++i) {
        int32_t axis = axes[i];
        NN_RET_CHECK(handleNegativeAxis(inputRank, &axis));
        shouldReduce[axis] = true;
    }

    // Input and output must have the same quantization parameters, etc.
    Shape outputShape = inputShape;
    outputShape.dimensions.clear();
    bool keepDims = context->getInputValue<bool8>(kInputKeepDims);
    for (uint32_t axis = 0; axis < inputRank; ++axis) {
        if (shouldReduce[axis]) {
            if (keepDims) {
                outputShape.dimensions.push_back(1);
            }
        } else {
            outputShape.dimensions.push_back(getSizeOfDimension(inputShape, axis));
        }
    }

    // Handle the case when all dimensions are removed
    if (outputShape.dimensions.empty()) {
        outputShape.dimensions.push_back(1);
    }

    return context->setOutputShape(kOutputTensor, outputShape);
}

bool executeProd(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return compute<_Float16>(context, 1, [](_Float16 a, _Float16 b) -> _Float16 {
                // Handle the zero case because 0 * inf evaluates to nan.
                if (a == 0 || b == 0) return 0;
                return a * b;
            });
        case OperandType::TENSOR_FLOAT32:
            return compute<float>(context, 1, [](float a, float b) -> float {
                // Handle the zero case because 0 * inf evaluates to nan.
                if (a == 0 || b == 0) return 0;
                return a * b;
            });
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation REDUCE_PROD";
    }
}

bool executeSum(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return compute<_Float16>(context, 0, [](_Float16 a, _Float16 b) { return a + b; });
        case OperandType::TENSOR_FLOAT32:
            return compute<float>(context, 0, [](float a, float b) { return a + b; });
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation REDUCE_SUM";
    }
}

bool executeMax(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return compute<_Float16>(context, kFloat16Lowest,
                                     [](_Float16 a, _Float16 b) { return std::max(a, b); });
        case OperandType::TENSOR_FLOAT32:
            return compute<float>(context, std::numeric_limits<float>::lowest(),
                                  [](float a, float b) { return std::max(a, b); });
        case OperandType::TENSOR_QUANT8_ASYMM:
            return compute<uint8_t>(context, std::numeric_limits<uint8_t>::lowest(),
                                    [](uint8_t a, uint8_t b) { return std::max(a, b); });
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return compute<int8_t>(context, std::numeric_limits<int8_t>::lowest(),
                                   [](int8_t a, int8_t b) { return std::max(a, b); });
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation REDUCE_MAX";
    }
}

bool executeMin(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return compute<_Float16>(context, kFloat16Max,
                                     [](_Float16 a, _Float16 b) { return std::min(a, b); });
        case OperandType::TENSOR_FLOAT32:
            return compute<float>(context, std::numeric_limits<float>::max(),
                                  [](float a, float b) { return std::min(a, b); });
        case OperandType::TENSOR_QUANT8_ASYMM:
            return compute<uint8_t>(context, std::numeric_limits<uint8_t>::max(),
                                    [](uint8_t a, uint8_t b) { return std::min(a, b); });
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return compute<int8_t>(context, std::numeric_limits<int8_t>::max(),
                                   [](int8_t a, int8_t b) { return std::min(a, b); });
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation REDUCE_MIN";
    }
}

bool executeAny(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_BOOL8:
            return compute<bool8>(context, false,
                                  [](bool8 a, bool8 b) { return static_cast<bool8>(a || b); });
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation REDUCE_ANY";
    }
}

bool executeAll(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_BOOL8:
            return compute<bool8>(context, true,
                                  [](bool8 a, bool8 b) { return static_cast<bool8>(a && b); });
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation REDUCE_ALL";
    }
}
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

}  // namespace reduce

NN_REGISTER_OPERATION_DEFAULT_VALIDATION(REDUCE_PROD, reduce::prepare, reduce::executeProd);
NN_REGISTER_OPERATION_DEFAULT_VALIDATION(REDUCE_SUM, reduce::prepare, reduce::executeSum);
NN_REGISTER_OPERATION_DEFAULT_VALIDATION(REDUCE_MAX, reduce::prepare, reduce::executeMax);
NN_REGISTER_OPERATION_DEFAULT_VALIDATION(REDUCE_MIN, reduce::prepare, reduce::executeMin);
NN_REGISTER_OPERATION_DEFAULT_VALIDATION(REDUCE_ANY, reduce::prepare, reduce::executeAny);
NN_REGISTER_OPERATION_DEFAULT_VALIDATION(REDUCE_ALL, reduce::prepare, reduce::executeAll);

}  // namespace nn
}  // namespace android
