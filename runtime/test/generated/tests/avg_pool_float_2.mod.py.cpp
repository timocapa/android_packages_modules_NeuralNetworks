// clang-format off
// Generated file (from: avg_pool_float_2.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace avg_pool_float_2 {
// Generated avg_pool_float_2 test
#include "generated/examples/avg_pool_float_2.example.cpp"
// Generated model constructor
#include "generated/models/avg_pool_float_2.model.cpp"
} // namespace avg_pool_float_2

TEST_F(GeneratedTests, avg_pool_float_2) {
    execute(avg_pool_float_2::CreateModel,
            avg_pool_float_2::is_ignored,
            avg_pool_float_2::get_examples());
}

#if 0
TEST_F(DynamicOutputShapeTests, avg_pool_float_2_dynamic_output_shape) {
    execute(avg_pool_float_2::CreateModel_dynamic_output_shape,
            avg_pool_float_2::is_ignored_dynamic_output_shape,
            avg_pool_float_2::get_examples_dynamic_output_shape());
}

#endif
