CC      = c++
CFLAGS  = -Wall -Werror -Wextra -std=c++98 -g3

# Mandatory
MAND_SRC_DIR = mandatory/srcs/
MAND_INC_DIR = mandatory/inc/
MAND_OBJ_DIR = mandatory/obj/
MAND_SRCS    = $(shell find $(MAND_SRC_DIR) -name '*.cpp')
MAND_OBJS    = $(MAND_SRCS:$(MAND_SRC_DIR)%.cpp=$(MAND_OBJ_DIR)%.o)
MAND_DEPS    = $(MAND_OBJS:.o=.d)

# Bonus
BONUS_SRC_DIR = bonus/srcs/
BONUS_INC_DIR = bonus/inc/
BONUS_OBJ_DIR = bonus/obj/
BONUS_SRCS    = $(shell find $(BONUS_SRC_DIR) -name '*.cpp')
BONUS_OBJS    = $(BONUS_SRCS:$(BONUS_SRC_DIR)%.cpp=$(BONUS_OBJ_DIR)%.o)
BONUS_DEPS    = $(BONUS_OBJS:.o=.d)

NAME       = ircserv
BONUS_NAME = ircbot

GREEN   = \033[1;32m
RED     = \033[1;31m
YELLOW  = \033[1;33m
BLUE    = \033[1;34m
CYAN    = \033[1;36m
NC      = \033[0m

all: $(NAME)
	@echo -e "$(CYAN)🚀 Partie mandatoire prête !$(NC)"

bonus: $(NAME) $(BONUS_NAME)
	@echo -e "$(CYAN)🌟 Bonus compilé avec succès !$(NC)"

$(NAME): $(MAND_OBJ_DIR) $(MAND_OBJS)
	@echo -e "$(YELLOW)⏳ Compilation de $(NAME)...$(NC)"
	@$(CC) $(CFLAGS) $(MAND_OBJS) -o $(NAME)
	@echo -e "$(GREEN)✓ Compilation de $(NAME) réussie!$(NC)"

$(BONUS_NAME): $(BONUS_OBJ_DIR) $(BONUS_OBJS)
	@echo -e "$(YELLOW)⏳ Compilation de $(BONUS_NAME)...$(NC)"
	@$(CC) $(CFLAGS) $(BONUS_OBJS) -o $(BONUS_NAME)
	@echo -e "$(GREEN)✓ Compilation de $(BONUS_NAME) réussie!$(NC)"

$(MAND_OBJ_DIR):
	@mkdir -p $(MAND_OBJ_DIR)
	@echo -e "$(BLUE)📁 Dossier mandatory/obj créé.$(NC)"

$(BONUS_OBJ_DIR):
	@mkdir -p $(BONUS_OBJ_DIR)
	@echo -e "$(BLUE)📁 Dossier bonus/obj créé.$(NC)"

$(MAND_OBJ_DIR)%.o: $(MAND_SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -I$(MAND_INC_DIR) -MMD -MP -c $< -o $@

$(BONUS_OBJ_DIR)%.o: $(BONUS_SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -I$(BONUS_INC_DIR) -MMD -MP -c $< -o $@

-include $(MAND_DEPS)
-include $(BONUS_DEPS)

clean:
	@rm -rf $(MAND_OBJ_DIR) $(BONUS_OBJ_DIR)
	@echo -e "$(RED)🧹 Fichiers objets supprimés.$(NC)"

fclean: clean
	@rm -f $(NAME) $(BONUS_NAME)
	@echo -e "$(RED)🗑️ Exécutables supprimés.$(NC)"

re: fclean all
	@echo -e "$(CYAN)🔁 Recompilation terminée.$(NC)"

.PHONY: all bonus clean fclean re
