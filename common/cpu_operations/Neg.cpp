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

#include "Neg.h"

#include <cmath>

#include "OperationResolver.h"
#include "OperationsExecutionUtils.h"
#include "Tracing.h"

namespace android {
namespace nn {
namespace neg {
namespace {

template <typename T>
inline bool compute(const T* input, const Shape& shape, T* output) {
    const auto size = getNumberOfElements(shape);
    for (uint32_t i = 0; i < size; ++i) {
        output[i] = -input[i];
    }
    return true;
}

}  // namespace

bool prepare(IOperationExecutionContext* context) {
    Shape input = context->getInputShape(kInputTensor);
    Shape output = context->getOutputShape(kOutputTensor);
    NN_RET_CHECK(SetShape(input, &output));
    return context->setOutputShape(kOutputTensor, output);
}

bool execute(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return compute(context->getInputBuffer<_Float16>(kInputTensor),
                           context->getInputShape(kInputTensor),
                           context->getOutputBuffer<_Float16>(kOutputTensor));
        case OperandType::TENSOR_FLOAT32:
            return compute(context->getInputBuffer<float>(kInputTensor),
                           context->getInputShape(kInputTensor),
                           context->getOutputBuffer<float>(kOutputTensor));
        case OperandType::TENSOR_INT32:
            return compute(context->getInputBuffer<int32_t>(kInputTensor),
                           context->getInputShape(kInputTensor),
                           context->getOutputBuffer<int32_t>(kOutputTensor));
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation " << kOperationName;
    }
}

}  // namespace neg

NN_REGISTER_OPERATION_DEFAULT_VALIDATION(NEG, neg::prepare, neg::execute);

}  // namespace nn
}  // namespace android
