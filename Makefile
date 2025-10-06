NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g
INCLUDES = -Isrc -Isrc/core -Isrc/http -Isrc/config -Isrc/utils -Isrc/cgi

SRCDIR = src
OBJDIR = obj
DIRS = $(OBJDIR) $(OBJDIR)/core $(OBJDIR)/http $(OBJDIR)/config $(OBJDIR)/utils $(OBJDIR)/cgi

SOURCES = main.cpp \
          core/Server.cpp \
          core/Client.cpp \
          core/Epoll.cpp \
          http/HTTPRequest.cpp \
          http/HTTPResponse.cpp \
          http/HTTPParser.cpp \
		  http/FileServer.cpp \
		  http/PostHandler.cpp \
          config/Config.cpp \
          config/ServerConfig.cpp \
          utils/Logger.cpp \
          utils/Utils.cpp \
		  cgi/CGIHandler.cpp

OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[0;33m
NC = \033[0m

all: $(NAME)

$(NAME): $(DIRS) $(OBJECTS)
	@echo "$(YELLOW)Linking $(NAME)...$(NC)"
	@$(CXX) $(OBJECTS) -o $(NAME) $(LDFLAGS)
	@echo "$(GREEN)$(NAME) compiled successfully!$(NC)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "$(YELLOW)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(DIRS):
	@mkdir -p $@

clean:
	@echo "$(RED)Cleaning object files...$(NC)"
	@rm -rf $(OBJDIR)

fclean: clean
	@echo "$(RED)Cleaning $(NAME)...$(NC)"
	@rm -f $(NAME)

re: fclean all

# Dev helpers
test: $(NAME)
	@echo "$(GREEN)Starting test server on port 8080...$(NC)"
	@./$(NAME) webserv.conf

#debug: CXXFLAGS += -DDEBUG_MODE -fsanitize=address
#debug: LDFLAGS += -fsanitize=address
debug: CXXFLAGS += -DDEBUG_MODE
debug: $(NAME)

valgrind: $(NAME)
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME) webserv.conf

# Check if all source files exist
check:
	@echo "$(YELLOW)Checking source files...$(NC)"
	@for file in $(SOURCES); do \
		if [ ! -f "$(SRCDIR)/$$file" ]; then \
			echo "$(RED)Missing: $(SRCDIR)/$$file$(NC)"; \
		else \
			echo "$(GREEN)Found: $(SRCDIR)/$$file$(NC)"; \
		fi; \
	done

.PHONY: all clean fclean re test debug valgrind check