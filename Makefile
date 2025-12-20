CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -Isrc -MMD -MP -O3
LDFLAGS  := 

SRC_DIR   := src
BUILD_DIR := build
TARGET    := server

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run debug release

all: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG
release: all

debug: CXXFLAGS += -g -O0 -fsanitize=address,undefined
debug: LDFLAGS  += -fsanitize=address,undefined
debug: all

$(TARGET): $(OBJS)
	@echo "[LINK] $@"
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX]  $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@echo "[CLEAN]"
	@rm -rf $(BUILD_DIR) $(TARGET)

run: all
	@./$(TARGET)
