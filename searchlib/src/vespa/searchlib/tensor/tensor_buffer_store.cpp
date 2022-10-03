// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "tensor_buffer_store.h"
#include <vespa/eval/eval/value_codec.h>
#include <vespa/eval/streamed/streamed_value_builder_factory.h>
#include <vespa/vespalib/datastore/array_store.hpp>
#include <vespa/vespalib/datastore/buffer_type.hpp>
#include <vespa/vespalib/datastore/datastore.hpp>
#include <vespa/vespalib/util/size_literals.h>

using vespalib::alloc::MemoryAllocator;
using vespalib::datastore::EntryRef;
using vespalib::eval::StreamedValueBuilderFactory;
using vespalib::eval::Value;
using vespalib::eval::ValueType;

namespace search::tensor {

namespace {

constexpr float ALLOC_GROW_FACTOR = 0.2;

}

TensorBufferStore::TensorBufferStore(const ValueType& tensor_type, std::shared_ptr<MemoryAllocator> allocator, uint32_t max_small_subspaces_type_id)
    : TensorStore(ArrayStoreType::get_data_store_base(_array_store)),
      _tensor_type(tensor_type),
      _ops(_tensor_type),
      _array_store(ArrayStoreType::optimizedConfigForHugePage(max_small_subspaces_type_id,
                                                              TensorBufferTypeMapper(max_small_subspaces_type_id, &_ops),
                                                              MemoryAllocator::HUGEPAGE_SIZE, 4_Ki, 8_Ki, ALLOC_GROW_FACTOR),
                   std::move(allocator), TensorBufferTypeMapper(max_small_subspaces_type_id, &_ops)),
      _add_buffer()
{
}

TensorBufferStore::~TensorBufferStore() = default;

void
TensorBufferStore::holdTensor(EntryRef ref)
{
    _array_store.remove(ref);
}

EntryRef
TensorBufferStore::move(EntryRef ref)
{
    if (!ref.valid()) {
        return EntryRef();
    }
    auto buf = _array_store.get(ref);
    auto new_ref = _array_store.add(buf);
    _ops.copied_labels(buf);
    _array_store.remove(ref);
    return new_ref;
}

EntryRef
TensorBufferStore::store_tensor(const Value &tensor)
{
    uint32_t num_subspaces = tensor.index().size();
    auto array_size = _ops.get_array_size(num_subspaces);
    _add_buffer.resize(array_size);
    _ops.store_tensor(_add_buffer, tensor);
    return _array_store.add(_add_buffer);
}

EntryRef
TensorBufferStore::store_encoded_tensor(vespalib::nbostream &encoded)
{
    const auto &factory = StreamedValueBuilderFactory::get();
    auto val = vespalib::eval::decode_value(encoded, factory);
    return store_tensor(*val);
}

std::unique_ptr<Value>
TensorBufferStore::get_tensor(EntryRef ref) const
{
    if (!ref.valid()) {
        return {};
    }
    auto buf = _array_store.get(ref);
    return _ops.make_fast_view(buf, _tensor_type);
}

bool
TensorBufferStore::encode_stored_tensor(EntryRef ref, vespalib::nbostream &target) const
{
    if (!ref.valid()) {
        return false;
    }
    auto buf = _array_store.get(ref);
    _ops.encode_stored_tensor(buf, _tensor_type, target);
    return true;
}

}