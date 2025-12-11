#!/bin/bash

# does not wirk by new securiy model
export DYLD_FALLBACK_LIBRARY_PATH=$HOME/boost/boost_1_84_0/dist/lib

install_name_tool -add_rpath $HOME/boost/boost_1_84_0/dist/lib src/qt/test/test_pivx-qt
install_name_tool -add_rpath $HOME/boost/boost_1_84_0/dist/lib src/test/test_pivx
install_name_tool -add_rpath $HOME/boost/boost_1_84_0/dist/lib src/pivx-tx
install_name_tool -add_rpath $HOME/boost/boost_1_84_0/dist/lib $HOME/git/PIVX/src/pivxd

otool -l src/qt/test/test_pivx-qt | grep -A10 RPATH
otool -l src/test/test_pivx | grep -A3 RPATH
otool -l src/pivx-tx | grep -A3 RPATH
otool -l src/pivxd | grep -A3 RPATH

make check
