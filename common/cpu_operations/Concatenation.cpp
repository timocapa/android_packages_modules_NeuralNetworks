/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "Concatenation.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "OperationResolver.h"
#include "OperationsExecutionUtils.h"
#include "Tracing.h"
#include "nnapi/Validation.h"

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Winvalid-partial-specialization"
#include <tensorflow/lite/kernels/internal/optimized/legacy_optimized_ops.h>
#include <tensorflow/lite/kernels/internal/reference/legacy_reference_ops.h>
#include <tensorflow/lite/kernels/internal/reference/reference_ops.h>
#include <tensorflow/lite/kernels/internal/types.h>
#pragma clang diagnostic pop

#include "CpuOperationUtils.h"
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

namespace android {
namespace nn {
namespace concatenation {

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
namespace {

template <typename T>
bool concatenation(const std::vector<const T*>& inputDataPtrs,
                   const std::vector<Shape>& inputShapes, int32_t axis, T* outputData,
                   const Shape& outputShape) {
    NNTRACE_TRANS("concatenation");
    int num_inputs = inputShapes.size();
    std::vector<tflite::Dims<4>*> inputDimsPtr(num_inputs);
    std::vector<tflite::Dims<4>> inputDims(num_inputs);
    for (int i = 0; i < num_inputs; i++) {
        inputDims[i] = convertShapeToDims(inputShapes[i]);
        inputDimsPtr[i] = &inputDims[i];
    }
    NNTRACE_COMP_SWITCH("optimized_ops::Concatenation");
    tflite::optimized_ops::Concatenation<tflite::FusedActivationFunctionType::kNone, T>(
            getNumberOfDimensions(outputShape) - axis - 1, inputDataPtrs.data(),
            inputDimsPtr.data(), num_inputs, outputData, convertShapeToDims(outputShape));

    return true;
}

template <>
bool concatenation<uint8_t>(const std::vector<const uint8_t*>& inputDataPtrs,
                            const std::vector<Shape>& inputShapes, int32_t axis,
                            uint8_t* outputData, const Shape& outputShape) {
    NNTRACE_TRANS("concatenationQuant8");
    int num_inputs = inputShapes.size();
    std::vector<float> inputScales(num_inputs);
    std::vector<int32> inputOffsets(num_inputs);
    std::vector<tflite::Dims<4>*> inputDimsPtr(num_inputs);
    std::vector<tflite::Dims<4>> inputDims(num_inputs);
    for (int i = 0; i < num_inputs; i++) {
        inputScales[i] = inputShapes[i].scale;
        inputOffsets[i] = inputShapes[i].offset;
        inputDims[i] = convertShapeToDims(inputShapes[i]);
        inputDimsPtr[i] = &inputDims[i];
    }

    NNTRACE_COMP_SWITCH("reference_ops::Concatenation");
    tflite::reference_ops::Concatenation(
            getNumberOfDimensions(outputShape) - axis - 1, inputDataPtrs.data(),
            inputDimsPtr.data(), inputOffsets.data(), inputScales.data(), num_inputs, outputData,
            convertShapeToDims(outputShape), outputShape.offset, outputShape.scale);

    return true;
}

template <typename T>
inline bool concatenation(IOperationExecutionContext* context) {
    uint32_t inputCount = context->getNumInputs() - 1;
    std::vector<const T*> inputDatas;
    std::vector<Shape> inputShapes;
    for (uint32_t i = 0; i < inputCount; ++i) {
        const T* buffer = context->getInputBuffer<T>(i);
        if (buffer == nullptr) continue;
        inputDatas.push_back(buffer);
        inputShapes.push_back(context->getInputShape(i));
    }
    return concatenation(inputDatas, inputShapes, context->getInputValue<int32_t>(inputCount),
                         context->getOutputBuffer<T>(kOutputTensor),
                         context->getOutputShape(kOutputTensor));
}

template <>
inline bool concatenation<int8_t>(IOperationExecutionContext* context) {
    uint32_t inputCount = context->getNumInputs() - 1;
    std::vector<std::vector<uint8_t>> inputs_uint8(inputCount);
    for (uint32_t i = 0; i < inputCount; ++i) {
        const auto currentSize = getNumberOfElements(context->getInputShape(i));
        inputs_uint8[i].resize(currentSize);
        if (currentSize != 0) {
            convertInt8ToUInt8(context->getInputBuffer<int8_t>(i), &inputs_uint8[i]);
        }
    }
    std::vector<const uint8_t*> inputDatas;
    std::vector<Shape> inputShapes;
    for (uint32_t i = 0; i < inputCount; ++i) {
        inputDatas.push_back(inputs_uint8[i].data());
        inputShapes.push_back(context->getInputShape(i));
        inputShapes[i].offset += 128;
    }

    std::vector<uint8_t> output_uint8(getNumberOfElements(context->getOutputShape(kOutputTensor)));
    Shape outputShape(context->getOutputShape(kOutputTensor));
    outputShape.offset += 128;
    NN_RET_CHECK(concatenation(inputDatas, inputShapes, context->getInputValue<int32_t>(inputCount),
                               output_uint8.data(), outputShape));

    convertUInt8ToInt8(output_uint8, context->getOutputBuffer<int8_t>(kOutputTensor));

    return true;
}

}  // namespace

bool prepare(IOperationExecutionContext* context) {
    uint32_t numInputs = context->getNumInputs();
    NN_RET_CHECK_GE(numInputs, 2u);
    const Shape& input0 = context->getInputShape(0);
    uint32_t numDimensions = getNumberOfDimensions(input0);
    int32_t axis = context->getInputValue<int32_t>(numInputs - 1);
    NN_RET_CHECK_GE(axis, 0);
    NN_RET_CHECK_LT(static_cast<uint32_t>(axis), numDimensions);
    NN_RET_CHECK_LE(numDimensions, 4u);

    uint32_t sumAxis = getSizeOfDimension(input0, axis);
    for (uint32_t i = 1; i < numInputs - 1; ++i) {
        const Shape& input = context->getInputShape(i);
        NN_RET_CHECK_EQ(getNumberOfDimensions(input), numDimensions);
        NN_RET_CHECK(input.type == input0.type);
        for (uint32_t d = 0; d < numDimensions; ++d) {
            if (d == static_cast<uint32_t>(axis)) {
                sumAxis += getSizeOfDimension(input, axis);
            } else {
                NN_RET_CHECK_EQ(getSizeOfDimension(input0, d), getSizeOfDimension(input, d));
            }
        }
    }

    Shape output = context->getOutputShape(kOutputTensor);
    output.type = input0.type;
    output.dimensions = input0.dimensions;
    output.dimensions[axis] = sumAxis;
    return context->setOutputShape(kOutputTensor, output);
}

bool execute(IOperationExecutionContext* context) {
    // Bypass execution in the case of zero-sized input.
    if (getNumberOfElements(context->getOutputShape(kOutputTensor)) == 0) return true;
    switch (context->getInputType(0)) {
        case OperandType::TENSOR_FLOAT16:
            return concatenation<_Float16>(context);
        case OperandType::TENSOR_FLOAT32:
            return concatenation<float>(context);
        case OperandType::TENSOR_QUANT8_ASYMM:
            return concatenation<uint8_t>(context);
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return concatenation<int8_t>(context);
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation " << kOperationName;
    }
}
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

}  // namespace concatenation

NN_REGISTER_OPERATION_DEFAULT_VALIDATION(CONCATENATION, concatenation::prepare,
                                         concatenation::execute, .allowZeroSizedInput = true);

}  // namespace nn
}  // namespace android
