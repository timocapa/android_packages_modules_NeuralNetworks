// Generated from space_to_batch_quant8_nonzero.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"
using namespace test_helper;  // NOLINT(google-build-using-namespace)

namespace generated_tests::space_to_batch_quant8_nonzero {

const TestModel& get_test_model() {
    static TestModel model = {
        .main = {
                .operands = {{ // input
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1, 5, 2, 1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
                        }, { // block_size
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({3, 2})
                        }, { // paddings
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2, 2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0})
                        }, { // output
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {6, 2, 2, 1},
                            .numberOfConsumers = 0,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({9, 9, 9, 5, 9, 9, 9, 6, 9, 1, 9, 7, 9, 2, 9, 8, 9, 3, 9, 9, 9, 4, 9, 10})
                        }},
                .operations = {{
                            .type = TestOperationType::SPACE_TO_BATCH_ND,
                            .inputs = {0, 1, 2},
                            .outputs = {3}
                        }},
                .inputIndexes = {0},
                .outputIndexes = {3}
            },
        .referenced = {},
        .isRelaxed = false,
        .expectedMultinomialDistributionTolerance = 0,
        .expectFailure = false,
        .minSupportedVersion = TestHalVersion::V1_2
    };
    return model;
}

const auto dummy_test_model = TestModelManager::get().add("space_to_batch_quant8_nonzero", get_test_model());

}  // namespace generated_tests::space_to_batch_quant8_nonzero

namespace generated_tests::space_to_batch_quant8_nonzero {

const TestModel& get_test_model_all_inputs_as_internal() {
    static TestModel model = {
        .main = {
                .operands = {{ // input
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1, 5, 2, 1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({})
                        }, { // block_size
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({3, 2})
                        }, { // paddings
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2, 2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0})
                        }, { // output
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {6, 2, 2, 1},
                            .numberOfConsumers = 0,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({9, 9, 9, 5, 9, 9, 9, 6, 9, 1, 9, 7, 9, 2, 9, 8, 9, 3, 9, 9, 9, 4, 9, 10})
                        }, { // input_new
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1, 5, 2, 1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
                        }, { // placeholder
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({9})
                        }, { // param
                            .type = TestOperandType::INT32,
                            .dimensions = {},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({0})
                        }},
                .operations = {{
                            .type = TestOperationType::ADD,
                            .inputs = {4, 5, 6},
                            .outputs = {0}
                        }, {
                            .type = TestOperationType::SPACE_TO_BATCH_ND,
                            .inputs = {0, 1, 2},
                            .outputs = {3}
                        }},
                .inputIndexes = {4},
                .outputIndexes = {3}
            },
        .referenced = {},
        .isRelaxed = false,
        .expectedMultinomialDistributionTolerance = 0,
        .expectFailure = false,
        .minSupportedVersion = TestHalVersion::V1_2
    };
    return model;
}

const auto dummy_test_model_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_quant8_nonzero_all_inputs_as_internal", get_test_model_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_quant8_nonzero

namespace generated_tests::space_to_batch_quant8_nonzero {

const TestModel& get_test_model_all_tensors_as_inputs() {
    static TestModel model = {
        .main = {
                .operands = {{ // input
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1, 5, 2, 1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
                        }, { // block_size
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({3, 2})
                        }, { // paddings
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2, 2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0})
                        }, { // output
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {6, 2, 2, 1},
                            .numberOfConsumers = 0,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({9, 9, 9, 5, 9, 9, 9, 6, 9, 1, 9, 7, 9, 2, 9, 8, 9, 3, 9, 9, 9, 4, 9, 10})
                        }},
                .operations = {{
                            .type = TestOperationType::SPACE_TO_BATCH_ND,
                            .inputs = {0, 1, 2},
                            .outputs = {3}
                        }},
                .inputIndexes = {0, 1, 2},
                .outputIndexes = {3}
            },
        .referenced = {},
        .isRelaxed = false,
        .expectedMultinomialDistributionTolerance = 0,
        .expectFailure = false,
        .minSupportedVersion = TestHalVersion::V1_2
    };
    return model;
}

const auto dummy_test_model_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_quant8_nonzero_all_tensors_as_inputs", get_test_model_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_quant8_nonzero

namespace generated_tests::space_to_batch_quant8_nonzero {

const TestModel& get_test_model_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .main = {
                .operands = {{ // input
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1, 5, 2, 1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({})
                        }, { // block_size
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({3, 2})
                        }, { // paddings
                            .type = TestOperandType::TENSOR_INT32,
                            .dimensions = {2, 2},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0})
                        }, { // output
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {6, 2, 2, 1},
                            .numberOfConsumers = 0,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({9, 9, 9, 5, 9, 9, 9, 6, 9, 1, 9, 7, 9, 2, 9, 8, 9, 3, 9, 9, 9, 4, 9, 10})
                        }, { // input_new
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1, 5, 2, 1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
                        }, { // placeholder1
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .dimensions = {1},
                            .numberOfConsumers = 1,
                            .scale = 1.0f,
                            .zeroPoint = 9,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<uint8_t>({9})
                        }, { // param1
                            .type = TestOperandType::INT32,
                            .dimensions = {},
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .zeroPoint = 0,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .channelQuant = {},
                            .isIgnored = false,
                            .data = TestBuffer::createFromVector<int32_t>({0})
                        }},
                .operations = {{
                            .type = TestOperationType::ADD,
                            .inputs = {4, 5, 6},
                            .outputs = {0}
                        }, {
                            .type = TestOperationType::SPACE_TO_BATCH_ND,
                            .inputs = {0, 1, 2},
                            .outputs = {3}
                        }},
                .inputIndexes = {1, 2, 4},
                .outputIndexes = {3}
            },
        .referenced = {},
        .isRelaxed = false,
        .expectedMultinomialDistributionTolerance = 0,
        .expectFailure = false,
        .minSupportedVersion = TestHalVersion::V1_2
    };
    return model;
}

const auto dummy_test_model_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_quant8_nonzero_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_quant8_nonzero

