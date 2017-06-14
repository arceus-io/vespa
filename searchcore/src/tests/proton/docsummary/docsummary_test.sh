#!/bin/bash
# Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
set -e
rm -rf tmp
rm -rf tmpdb
rm -rf summary
rm -rf indexingdocument
rm -rf searchdocument
rm -rf *.dat
$VALGRIND ./searchcore_docsummary_test_app
rm -rf tmp
rm -rf tmpdb
rm -rf summary
rm -rf indexingdocument
rm -rf searchdocument
rm -rf *.dat
$VALGRIND ./searchcore_summaryfieldconverter_test_app
