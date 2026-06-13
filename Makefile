NAME			:= webserv


CXX				:= c++
DEBUG_SYMBOLS	:= -g3
CXXFLAGS		:= -Wall -Wextra -Werror -std=c++98 ${DEBUG_SYMBOLS}
DEPFLAGS		:= -MMD -MP


SANITIZE_FLAGS	:= -fsanitize=address
VALGRIND_FLAGS	:= --track-origins=yes -s --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all


SRC_DIR			:= ./src/
INC_DIR			:= ./include/
OBJ_DIR			:= ./obj/


CGI_DIR			:= ${SRC_DIR}cgi/
CLNT_DIR		:= ${SRC_DIR}client/
CONF_DIR		:= ${SRC_DIR}config/
HTTP_DIR		:= ${SRC_DIR}http/
LOG_DIR			:= ${SRC_DIR}log/
SERV_DIR		:= ${SRC_DIR}server/


CGI_SRC_FILES	:=	CgiHandler.cpp
CLNT_SRC_FILES	:=	Client.cpp
CONF_SRC_FILES	:=	LocationConfig.cpp \
					ServerConfig.cpp \
					Config.cpp
HTTP_SRC_FILES	:=	HttpResponse.cpp \
					RequestContext.cpp
LOG_SRC_FILES	:=	log.cpp
ROOT_SRC_FILES	:=	main.cpp
SERV_SRC_FILES	:=	Server.cpp \
					WebServer.cpp

CGI_SRCS		:= $(addprefix ${CGI_DIR}, ${CGI_SRC_FILES})
CLNT_SRCS		:= $(addprefix ${CLNT_DIR}, ${CLNT_SRC_FILES})
CONF_SRCS		:= $(addprefix ${CONF_DIR}, ${CONF_SRC_FILES})
HTTP_SRCS		:= $(addprefix ${HTTP_DIR}, ${HTTP_SRC_FILES})
LOG_SRCS		:= $(addprefix ${LOG_DIR}, ${LOG_SRC_FILES})
ROOT_SRCS		:= $(addprefix ${SRC_DIR}, ${ROOT_SRC_FILES})
SERV_SRCS		:= $(addprefix ${SERV_DIR}, ${SERV_SRC_FILES})


SRC_FILES		:=	${ROOT_SRCS} \
					${CONF_SRCS} \
					${SERV_SRCS} \
					${CLNT_SRCS} \
					${HTTP_SRCS} \
					${LOG_SRCS} \
					${CGI_SRCS}


# "patsubst": pattern substitution
# parameters: pattern, replacement, text
#
# pattern: the pattern to match. Supports wildcards
# replacement: the string to replace the pattern with. By using wildcards,
#              Make keeps the original text matched by the same
#              wildcard in the pattern
# text: the list of strings on which the substitution will be performed
OBJ_FILES		:= ${patsubst ${SRC_DIR}%.cpp, ${OBJ_DIR}%.o, ${SRC_FILES}}
DEPS			:= $(OBJ_FILES:.o=.d)


INCLUDES		:= -I $(INC_DIR)


GREEN			:= \033[0;32m
CYAN			:= \033[0;36m
RESET			:= \033[0m


all: $(NAME)


$(NAME): $(OBJ_FILES)
	@$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $(NAME)
	@printf "$(GREEN)==> Built $(NAME)$(RESET)\n"


$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(RESET) $<\n"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@


clean:
	@rm -f ${OBJ_FILES}
	@rm -rf $(OBJ_DIR)
	@printf "$(GREEN)==> Cleaned object files$(RESET)\n"


fclean: clean
	@rm -f $(NAME)
	@printf "$(GREEN)==> Removed $(NAME)$(RESET)\n"


re: fclean all


HEADERS_TEST	:= test_headers
BODY_TEST		:= test_body
REQLINE_TEST	:= test_request_line
PARSER_SRCS		:= ${SRC_DIR}request.cpp ${SRC_DIR}RequestLine.cpp

test:
	@$(CXX) $(CXXFLAGS) $(INCLUDES) ${PARSER_SRCS} ./tests/test_headers.cpp -o $(HEADERS_TEST)
	@./$(HEADERS_TEST)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) ${PARSER_SRCS} ./tests/test_body.cpp -o $(BODY_TEST)
	@./$(BODY_TEST)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) ${PARSER_SRCS} ./tests/test_request_line.cpp -o $(REQLINE_TEST)
	@./$(REQLINE_TEST)


sanitize: ${OBJ_FILES}
	@${CXX} ${CXXFLAGS} ${SANITIZE_FLAGS} ${OBJ_FILES} -o ${NAME}
	@echo "CPP compiler's sanitizer has been added to debug memory issues."


valgrind:
	@valgrind ${VALGRIND_FLAGS} ./${NAME}


gdb:
	@gdb ./${NAME}


help:
	@echo "Available targets:"
	@echo "    all            - Build the project (default)"
	@echo "    clean          - Remove object files"
	@echo "    fclean         - Remove object files and the executable"
	@echo "    re             - Rebuild the project"
	@echo "    sanitize       - Build with address sanitizer for debugging"
	@echo "    valgrind       - Run the program with valgrind"
	@echo "    gdb            - Run the program with gdb"


-include $(DEPS)


.PHONY: all clean fclean re sanitize valgrind gdb help
