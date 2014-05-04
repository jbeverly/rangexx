#!/bin/bash -x 
# Reconfigure with gcov support

find . -iname '*.gcno' | xargs rm

MAKE=make
if [ -x ./config.status ]; then
	FLAGS='-ggdb -O0 --coverage -D_ENABLE_TESTING'
	oldconfigure=$(./config.status --config)
	./config.status --config | grep -q -- "${FLAGS}" || eval 'CXXFLAGS="'"$FLAGS"'" CFLAGS="'"$FLAGS"'" '"./configure $(./config.status --config)"
else
	CXXFLAGS="-ggdb -O0 --coverage" CFLAGS="-ggdb -O0 --coverage" ./configure
fi

# Generate gcov output
${MAKE} clean
${MAKE}

# Generate html report
lcov --zerocounters -q --directory $(pwd)
lcov --directory $(pwd) --capture --initial -o rangexx_test.info
${MAKE} check
lcov --no-checksum --directory $(pwd) --capture --output-file rangexx_test.info
lcov --remove rangexx_test.info "/usr*" -o rangexx_test.info
lcov --remove rangexx_test.info "/opt*" -o rangexx_test.info
lcov --remove rangexx_test.info "*.pb.*" -o rangexx_test.info
lcov --remove rangexx_test.info "*.ycpp*" -o rangexx_test.info
lcov --remove rangexx_test.info "*.lcpp*" -o rangexx_test.info
rm -rf $(pwd)/test_coverage
genhtml -o $(pwd)/test_coverage -t "range++ test coverage" --num-spaces 4 rangexx_test.info

find . -iname '*.gcno' -o -iname '*.gcda' | xargs rm
rm rangexx_test.info

make distclean
eval ./configure $oldconfigure
