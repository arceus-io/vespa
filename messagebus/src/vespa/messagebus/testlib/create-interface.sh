# Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once
#!/bin/sh

class=$1
guard=`echo $class | tr 'a-z' 'A-Z'`

cat <<EOF
// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


namespace mbus {

class $class
{
public:
    virtual ~$class() {}
};

} // namespace mbus

EOF
