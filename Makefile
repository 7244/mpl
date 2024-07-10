OUTPUT = a.exe

LINK =
INCLUDE = 
CS = -Wall -Wextra -std=c++2a -Wno-unused-parameter -Wno-sign-compare -Wno-shift-op-parentheses

debug:
	clang++ $(CS) -g main.cpp -o $(OUTPUT) $(INCLUDE) $(LINK)

release:
	clang++ $(CS) -s -O3 main.cpp -o $(OUTPUT) $(INCLUDE) $(LINK)

clean:
	rm $(OUTPUT)
