// clang-format off
// Generated file (from: strided_slice_float_8.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace strided_slice_float_8 {
// Generated strided_slice_float_8 test
#include "generated/examples/strided_slice_float_8.example.cpp"
// Generated model constructor
#include "generated/models/strided_slice_float_8.model.cpp"
} // namespace strided_slice_float_8

TEST_F(GeneratedTests, strided_slice_float_8) {
    execute(strided_slice_float_8::CreateModel,
            strided_slice_float_8::is_ignored,
            strided_slice_float_8::get_examples());
}

