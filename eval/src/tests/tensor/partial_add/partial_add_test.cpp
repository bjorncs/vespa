// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/eval/eval/simple_value.h>
#include <vespa/eval/eval/test/tensor_model.hpp>
#include <vespa/eval/eval/value_codec.h>
#include <vespa/eval/tensor/cell_values.h>
#include <vespa/eval/tensor/default_tensor_engine.h>
#include <vespa/eval/tensor/partial_update.h>
#include <vespa/eval/tensor/sparse/sparse_tensor.h>
#include <vespa/eval/tensor/tensor.h>
#include <vespa/vespalib/util/stringfmt.h>
#include <vespa/vespalib/gtest/gtest.h>
#include <optional>

using namespace vespalib;
using namespace vespalib::eval;
using namespace vespalib::eval::test;

using vespalib::make_string_short::fmt;

std::vector<Layout> add_layouts = {
    {x({"a"})},                           {x({"b"})},
    {x({"a","b"})},                       {x({"a","c"})},
    float_cells({x({"a","b"})}),          {x({"a","c"})},
    {x({"a","b"})},                       float_cells({x({"a","c"})}),
    float_cells({x({"a","b"})}),          float_cells({x({"a","c"})}),
    {x({"a","b","c"}),y({"d","e"})},      {x({"b","f"}),y({"d","g"})},             
    {x(3),y({"a","b"})},                  {x(3),y({"b","c"})}
};

TensorSpec reference_add(const TensorSpec &a, const TensorSpec &b) {
    TensorSpec result(a.type());
    for (const auto &cell: b.cells()) {
        result.add(cell.first, cell.second);
    }
    auto end_iter = b.cells().end();
    for (const auto &cell: a.cells()) {
        auto iter = b.cells().find(cell.first);
        if (iter == end_iter) {
            result.add(cell.first, cell.second);
        }
    }
    return result;
}

TensorSpec perform_partial_add(const TensorSpec &a, const TensorSpec &b) {
    const auto &factory = SimpleValueBuilderFactory::get();
    auto lhs = value_from_spec(a, factory);
    auto rhs = value_from_spec(b, factory);
    auto up = tensor::TensorPartialUpdate::add(*lhs, *rhs, factory);
    if (up) {
        return spec_from_value(*up);
    } else {
        return TensorSpec(a.type());
    }
}

TensorSpec perform_old_add(const TensorSpec &a, const TensorSpec &b) {
    const auto &engine = tensor::DefaultTensorEngine::ref();
    auto lhs = engine.from_spec(a);
    auto rhs = engine.from_spec(b);
    auto lhs_tensor = dynamic_cast<tensor::Tensor *>(lhs.get());
    EXPECT_TRUE(lhs_tensor);
    auto rhs_tensor = dynamic_cast<tensor::Tensor *>(rhs.get());
    EXPECT_TRUE(rhs_tensor);
    auto up = lhs_tensor->add(*rhs_tensor);
    EXPECT_TRUE(up);
    return engine.to_spec(*up);
}


TEST(PartialAddTest, partial_add_works_for_simple_values) {
    ASSERT_TRUE((add_layouts.size() % 2) == 0);
    for (size_t i = 0; i < add_layouts.size(); i += 2) {
        TensorSpec lhs = spec(add_layouts[i], N());
        TensorSpec rhs = spec(add_layouts[i + 1], Div16(N()));
        SCOPED_TRACE(fmt("\n===\nLHS: %s\nRHS: %s\n===\n", lhs.to_string().c_str(), rhs.to_string().c_str()));
        auto expect = reference_add(lhs, rhs);
        auto actual = perform_partial_add(lhs, rhs);
        EXPECT_EQ(actual, expect);
    }
}

TEST(PartialAddTest, partial_add_works_like_old_add) {
    ASSERT_TRUE((add_layouts.size() % 2) == 0);
    for (size_t i = 0; i < add_layouts.size(); i += 2) {
        TensorSpec lhs = spec(add_layouts[i], N());
        TensorSpec rhs = spec(add_layouts[i + 1], Div16(N()));
        SCOPED_TRACE(fmt("\n===\nLHS: %s\nRHS: %s\n===\n", lhs.to_string().c_str(), rhs.to_string().c_str()));
        auto expect = perform_old_add(lhs, rhs);
        auto actual = perform_partial_add(lhs, rhs);
        EXPECT_EQ(actual, expect);
        printf("%s add %s -> %s\n", lhs.to_string().c_str(), rhs.to_string().c_str(), actual.to_string().c_str());

    }
}

GTEST_MAIN_RUN_ALL_TESTS()
