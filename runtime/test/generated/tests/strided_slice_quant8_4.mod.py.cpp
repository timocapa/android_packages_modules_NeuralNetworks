// clang-format off
// Generated file (from: strided_slice_quant8_4.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace strided_slice_quant8_4 {
// Generated strided_slice_quant8_4 test
#include "generated/examples/strided_slice_quant8_4.example.cpp"
// Generated model constructor
#include "generated/models/strided_slice_quant8_4.model.cpp"
} // namespace strided_slice_quant8_4

TEST_F(GeneratedTests, strided_slice_quant8_4) {
    execute(strided_slice_quant8_4::CreateModel,
            strided_slice_quant8_4::is_ignored,
            strided_slice_quant8_4::get_examples());
}

#if 0
TEST_F(DynamicOutputShapeTests, strided_slice_quant8_4_dynamic_output_shape) {
    execute(strided_slice_quant8_4::CreateModel_dynamic_output_shape,
            strided_slice_quant8_4::is_ignored_dynamic_output_shape,
            strided_slice_quant8_4::get_examples_dynamic_output_shape());
}

#endif
