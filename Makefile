CXX := clang++
CXXFLAGS := -g -O3 -std=c++20 -Wall -Wextra -Werror -Wno-error=unused-parameter -pedantic
DEPS := $(shell find . -iname "*.d")
# SRCDIR=./src/ 

# scanner: scanner.o
# 	$(CXX) -o $@ $^

main: main.o names.o
	$(CXX) -o $@ $^ -fsanitize=address

test: test.o
	$(CXX) -o $@ $^ -fsanitize=address

%.o: %.cpp
	$(CXX) -c -MMD -MP -o $@ $< $(CXXFLAGS)

clean:
	rm -rf main *.o a.out

-include $(DEPS)

# $@ is the left side of the :
# $< is the first dependency
# $^ is the right side of the :