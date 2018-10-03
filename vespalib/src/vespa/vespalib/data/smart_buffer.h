// Copyright 2018 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "input.h"
#include "output.h"
#include <vespa/vespalib/util/alloc.h>

namespace vespalib {

/**
 * A somewhat smarter buffer compared to SimpleBuffer. Keeps track of
 * data in a continuous memory segment. Tries to limit copying of
 * data.
 **/
class SmartBuffer : public Input,
                    public Output
{
private:
    alloc::Alloc _data;
    size_t       _read_pos;
    size_t       _write_pos;

    const char *read_ptr() const { return (const char *)(_data.get()) + _read_pos; }
    size_t read_len() const { return (_write_pos - _read_pos); } 
    char *write_ptr() { return (char *)(_data.get()) + _write_pos; }
    size_t write_len() const { return (_data.size() - _write_pos); }
    size_t unused() const { return (_data.size() - read_len()); }
    void ensure_free(size_t bytes);

public:
    SmartBuffer(size_t initial_size);
    ~SmartBuffer();
    size_t capacity() const { return _data.size(); }
    Memory obtain() override;
    Input &evict(size_t bytes) override;
    WritableMemory reserve(size_t bytes) override;
    Output &commit(size_t bytes) override;
};

} // namespace vespalib
