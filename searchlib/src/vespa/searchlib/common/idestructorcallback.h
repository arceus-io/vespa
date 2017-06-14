// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

namespace search
{

/**
 * Interface for class that performs a callback when instance is
 * destroyed. Typically a shared pointer to an instance is passed
 * around to multiple worker threads that performs portions of a
 * larger task before dropping the shared pointer, triggering the
 * callback when all worker threads have completed.
 */
class IDestructorCallback
{
public:
    virtual ~IDestructorCallback() { }
};

} // namespace search
