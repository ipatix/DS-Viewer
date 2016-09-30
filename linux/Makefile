CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Wunreachable-code -std=c++0x -D NDEBUG -O3
BINARY = dsview
LIBS = -lm -lSDL2 -pthread

SRC_DIR = ../dsview
OBJ_DIR = obj

INCLUDE = -I ../portaudio/include

GREEN = \033[1;32m
RED = \033[1;31m
BROWN = \033[1;33m
WHITE = \033[1;37m
NCOL = \033[0m

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_FILES:.cpp=.o)))

.PHONY: all
all: $(BINARY)

.PHONY: clean
clean:
	@printf "[$(BROWN)Cleaning$(NCOL)] $(WHITE)$(OBJ_FILES)$(NCOL)\n"
	@rm -f $(OBJ_FILES)

$(BINARY): $(OBJ_FILES)
	@printf "[$(RED)Linking$(NCOL)] $(WHITE)$(BINARY)$(NCOL)\n"
	@gcc -o $@ $(CXXFLAGS) $^ $(LIBS) -lstdc++

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/*.h
	@printf "[$(GREEN)Compiling$(NCOL)] $(WHITE)$@$(NCOL)\n"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCLUDE)

