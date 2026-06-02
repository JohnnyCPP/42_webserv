NAME		:= webserv


CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98
DEPFLAGS	:= -MMD -MP

SRC_DIR		:= src
INC_DIR		:= include
OBJ_DIR		:= obj

SRCS		:= $(shell find $(SRC_DIR) -name '*.cpp')
OBJS		:= $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS		:= $(OBJS:.o=.d)

INCLUDES	:= -I$(INC_DIR)


GREEN		:= \033[0;32m
CYAN		:= \033[0;36m
RESET		:= \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@printf "$(GREEN)==> Built $(NAME)$(RESET)\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(RESET) $<\n"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@printf "$(GREEN)==> Cleaned object files$(RESET)\n"

fclean: clean
	@rm -f $(NAME)
	@printf "$(GREEN)==> Removed $(NAME)$(RESET)\n"

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
