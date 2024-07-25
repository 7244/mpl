OUTPUT = a.exe

LINK =
INCLUDE = 
CS = -Wall -Wextra -std=c++2a -Wno-unused-parameter -Wno-sign-compare -Wno-shift-op-parentheses -Wno-invalid-offsetof

debug:
	clang++ $(CS) -g src/main.cpp -o $(OUTPUT) $(INCLUDE) $(LINK)

release:
	clang++ $(CS) -s -O3 src/main.cpp -o $(OUTPUT) $(INCLUDE) $(LINK)

clean:
	rm $(OUTPUT)
