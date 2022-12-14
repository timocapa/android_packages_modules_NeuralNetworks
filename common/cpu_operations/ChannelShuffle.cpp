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

#include "ChannelShuffle.h"

#include "OperationResolver.h"
#include "OperationsExecutionUtils.h"
#include "Tracing.h"

namespace android {
namespace nn {
namespace channel_shuffle {

template <typename T>
inline bool eval(const T* inputData, const Shape& inputShape, int32_t numGroups, int32_t axis,
                 T* outputData) {
    const uint32_t outerSize = getNumberOfElements(inputShape, 0, axis);
    const uint32_t axisSize = getSizeOfDimension(inputShape, axis);
    const uint32_t innerSize =
            getNumberOfElements(inputShape, axis + 1, getNumberOfDimensions(inputShape));
    const uint32_t groupSize = axisSize / numGroups;
    for (uint32_t outer = 0; outer < outerSize; ++outer) {
        for (uint32_t inner = 0; inner < innerSize; ++inner) {
            const T* inputBase = inputData + outer * axisSize * innerSize + inner;
            T* outputBase = outputData + outer * axisSize * innerSize + inner;
            for (uint32_t i = 0; i < groupSize; i++) {
                for (uint32_t j = 0; j < static_cast<uint32_t>(numGroups);
                     j++, outputBase += innerSize) {
                    *outputBase = inputBase[innerSize * (i + j * groupSize)];
                }
            }
        }
    }
    return true;
}

bool prepare(IOperationExecutionContext* context) {
    Shape input = context->getInputShape(kInputTensor);
    int32_t numGroups = context->getInputValue<int32_t>(kNumGroups);
    int32_t axis = context->getInputValue<int32_t>(kInputAxis);
    NN_RET_CHECK(handleNegativeAxis(input, &axis));
    NN_RET_CHECK(numGroups > 0);
    NN_RET_CHECK(getSizeOfDimension(input, axis) % numGroups == 0);
    return context->setOutputShape(kOutputTensor, input);
}

bool execute(IOperationExecutionContext* context) {
    int32_t numGroups = context->getInputValue<int32_t>(kNumGroups);
    int32_t axis = context->getInputValue<int32_t>(kInputAxis);
    NN_RET_CHECK(handleNegativeAxis(context->getInputShape(kInputTensor), &axis));
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return eval(context->getInputBuffer<_Float16>(kInputTensor),
                        context->getInputShape(kInputTensor), numGroups, axis,
                        context->getOutputBuffer<_Float16>(kOutputTensor));
        case OperandType::TENSOR_FLOAT32:
            return eval(context->getInputBuffer<float>(kInputTensor),
                        context->getInputShape(kInputTensor), numGroups, axis,
                        context->getOutputBuffer<float>(kOutputTensor));
        case OperandType::TENSOR_QUANT8_ASYMM:
            return eval(context->getInputBuffer<uint8_t>(kInputTensor),
                        context->getInputShape(kInputTensor), numGroups, axis,
                        context->getOutputBuffer<uint8_t>(kOutputTensor));
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return eval(context->getInputBuffer<int8_t>(kInputTensor),
                        context->getInputShape(kInputTensor), numGroups, axis,
                        context->getOutputBuffer<int8_t>(kOutputTensor));
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation " << kOperationName;
    }
}

}  // namespace channel_shuffle

NN_REGISTER_OPERATION_DEFAULT_VALIDATION(CHANNEL_SHUFFLE, channel_shuffle::prepare,
                                         channel_shuffle::execute);

}  // namespace nn
}  // namespace android
