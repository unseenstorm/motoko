#******************************************************************************#
#                                                                              #
# Sorry, cmake made me crazy...                                                #
#                                                                              #
#******************************************************************************#

NAME =	motoko
# WARDEN_DUMP = wardendump

C_SRC =				internal.c		\
					bncs.c			\
					clientman.c		\
					hash.c			\
					moduleman.c		\
					packet.c		\
					gui.c			\
					d2gs.c			\
					main.c			\
					mcp.c

C_UTIL =			string.c		\
					file.c			\
					system.c		\
					config.c		\
					compat.c		\
					curses.c		\
					error.c			\
					net.c			\
					list.c

C_D2GS_MEPH =		d2gs_meph.c

C_D2GS_DCLONE =		d2gs_dclone.c

C_D2GS_RECORD =		d2gs_record.c	\
					replay.c

C_D2GS_CHICKEN =	d2gs_chicken.c

C_D2GS_PES =		d2gs_pes.c

C_D2GS_PICKIT =		d2gs_pickit.c

C_D2GS_CHEST =		d2gs_chest.c

C_D2GS_PATHING =	d2gs_pathing.c

C_MCP_LOGIN =		mcp_login.c

C_MCP_GAME =		mcp_game.c

C_BNCS_LOGIN =		bncs_login.c


# C_WARDEN =		wmodule.c		\
# 				wutil.c			\
# 				wardenc.c		\
# 				wcrypt.c		\
# 				winapi.c

# C_WARDEN_DUMP =	wardendump.c


CONFIG_DIR = tmp
PLUGIN_DIR = plugins
O_DIR = tmp
ROOT_DIR = ..
INSTALL_DIR = /usr/local/bin
HERE = $(shell pwd)

DEFINES = 	-D_export="__attribute__((visibility(\"default\")))"	\
			-DCONFIGDIR=\"$(HERE)/$(CONFIG_DIR)\"					\
			-DPLUGINDIR=\"$(HERE)/$(PLUGIN_DIR)\"


VPATH =	$(ROOT_DIR)/src\
:$(ROOT_DIR)/lib/util\
:$(ROOT_DIR)/lib/plugins/d2gs_meph\
:$(ROOT_DIR)/lib/plugins/d2gs_dclone\
:$(ROOT_DIR)/lib/plugins/d2gs_record\
:$(ROOT_DIR)/lib/plugins/d2gs_chicken\
:$(ROOT_DIR)/lib/plugins/d2gs_pes\
:$(ROOT_DIR)/lib/plugins/d2gs_pickit\
:$(ROOT_DIR)/lib/plugins/d2gs_chest\
:$(ROOT_DIR)/lib/plugins/d2gs_pathing\
:$(ROOT_DIR)/lib/plugins/d2gs_record/replay\
:$(ROOT_DIR)/lib/plugins/mcp_login\
:$(ROOT_DIR)/lib/plugins/mcp_game\
:$(ROOT_DIR)/lib/plugins/bncs_login

# :$(ROOT_DIR)/lib/libwardenc/src\
# :$(ROOT_DIR)/lib/libwardenc/wardendump

O_SRC = $(C_SRC:%.c=$(O_DIR)/%.o)
O_UTIL = $(C_UTIL:%.c=$(O_DIR)/%.o)
O_D2GS_MEPH = $(C_D2GS_MEPH:%.c=$(O_DIR)/%.o)
O_D2GS_DCLONE = $(C_D2GS_DCLONE:%.c=$(O_DIR)/%.o)
O_D2GS_RECORD = $(C_D2GS_RECORD:%.c=$(O_DIR)/%.o)
O_D2GS_CHICKEN = $(C_D2GS_CHICKEN:%.c=$(O_DIR)/%.o)
O_D2GS_PES = $(C_D2GS_PES:%.c=$(O_DIR)/%.o)
O_D2GS_PICKIT = $(C_D2GS_PICKIT:%.c=$(O_DIR)/%.o)
O_D2GS_CHEST = $(C_D2GS_CHEST:%.c=$(O_DIR)/%.o)
O_D2GS_PATHING = $(C_D2GS_PATHING:%.c=$(O_DIR)/%.o)
O_MCP_LOGIN = $(C_MCP_LOGIN:%.c=$(O_DIR)/%.o)
O_MCP_GAME = $(C_MCP_GAME:%.c=$(O_DIR)/%.o)
O_BNCS_LOGIN = $(C_BNCS_LOGIN:%.c=$(O_DIR)/%.o)

# O_WARDEN = $(C_WARDEN:%.c=$(O_DIR)/%.o)
# O_WARDEN_DUMP = $(C_WARDEN_DUMP:%.c=$(O_DIR)/%.o)

OBJS =	$(O_SRC)					\
		$(O_UTIL)					\
		$(O_D2GS_MEPH)				\
		$(O_D2GS_DCLONE)			\
		$(O_D2GS_RECORD)			\
		$(O_D2GS_CHICKEN)			\
		$(O_D2GS_PES)				\
		$(O_D2GS_PICKIT)			\
		$(O_D2GS_CHEST)				\
		$(O_D2GS_PATHING)			\
		$(O_MCP_LOGIN)				\
		$(O_MCP_GAME)				\
		$(O_BNCS_LOGIN)

# 		$(O_WARDEN)			\
#		$(O_WARDEN_DUMP)	\

DEPS =  $(OBJS:%.o=%.d)

LIB_UTIL = $(O_DIR)/libutil.a

LIB_D2GS_MEPH = $(PLUGIN_DIR)/d2gs_meph_plugin.so
LIB_D2GS_DCLONE = $(PLUGIN_DIR)/d2gs_dclone_plugin.so
LIB_D2GS_RECORD = $(PLUGIN_DIR)/d2gs_record_plugin.so
LIB_D2GS_CHICKEN = $(PLUGIN_DIR)/d2gs_chicken_plugin.so
LIB_D2GS_PES = $(PLUGIN_DIR)/d2gs_pes_plugin.so
LIB_D2GS_PICKIT = $(PLUGIN_DIR)/d2gs_pickit_plugin.so
LIB_D2GS_CHEST = $(PLUGIN_DIR)/d2gs_chest_plugin.so
LIB_D2GS_PATHING = $(PLUGIN_DIR)/d2gs_pathing_plugin.so
LIB_D2GS_REPLAY = $(PLUGIN_DIR)/d2gs_replay_plugin.so
LIB_MCP_LOGIN = $(PLUGIN_DIR)/mcp_login_plugin.so
LIB_MCP_GAME = $(PLUGIN_DIR)/mcp_game_plugin.so
LIB_BNCS_LOGIN = $(PLUGIN_DIR)/bncs_login_plugin.so

# LIB_WARDEN = $(PLUGIN_DIR)/warden_plugin.so


SHARED_LIBS =	$(LIB_D2GS_MEPH)		\
				$(LIB_D2GS_DCLONE)		\
				$(LIB_D2GS_RECORD)		\
				$(LIB_D2GS_CHICKEN)		\
				$(LIB_D2GS_PES)			\
				$(LIB_D2GS_PICKIT)		\
				$(LIB_D2GS_CHEST)		\
				$(LIB_D2GS_PATHING)		\
				$(LIB_MCP_LOGIN)		\
				$(LIB_MCP_GAME)			\
				$(LIB_BNCS_LOGIN)
#				$(LIB_WARDEN)		\

I_DIR = -I .											\
		-I $(ROOT_DIR)/src								\
		-I $(ROOT_DIR)/src/data							\
		-I $(ROOT_DIR)/lib/util/include					\
		-I $(ROOT_DIR)/lib/plugins/d2gs_pathing			\
		-I $(ROOT_DIR)/lib/plugins/d2gs_record/replay

#		-I $(ROOT_DIR)/lib/libwardenc					\
#		-I $(ROOT_DIR)/lib/libwardenc/src				\


LIBS =	-pthread -lopts -lm -lncurses -ldl -lrt # -lcrypto -lz

MAKE =		make -j
RM =		rm -rf
CP =		cp
LN =		ln -s
AR =		ar cru
RANLIB =	ranlib
MKDIR =		mkdir -p
CC =		$(shell clang --version >/dev/null 2>&1 && echo clang || echo gcc)
CFLAGS =	-Wall -Wextra -O2

WHITE =	\033[37;01m
RED =	\033[31;01m
GREEN =	\033[32;01m
BLUE =	\033[34;01m
BASIC =	\033[0m

.PHONY: all debug sanitize me_cry install clean fclean re

all:
	@$(MAKE) $(NAME) $(FLAGS)

debug: FLAGS = "CFLAGS = -g -ggdb"
debug: all

sanitize: FLAGS = "CFLAGS = -g -ggdb -fsanitize=address,undefined -ferror-limit=5"
sanitize: all

me_cry: FLAGS = "CFLAGS = -Wpedantic -Wshadow -Wconversion -Wcast-align \
-Wstrict-prototypes -Wmissing-prototypes -Wunreachable-code -Winit-self \
-Wmissing-declarations -Wfloat-equal -Wbad-function-cast -Wundef \
-Waggregate-return -Wstrict-overflow=5 -Wold-style-definition -Wpadded \
-Wredundant-decls -Wall -Werror -Wextra" #-Wcast-qual
me_cry: all

-include $(DEPS)

$(NAME): $(O_SRC) $(SHARED_LIBS) $(LIB_UTIL) #$(WARDEN_DUMP)
	@echo "$(BLUE) $(O_SRC) $(GREEN)$(LIB_UTIL) $(WHITE)->$(RED) $@ $(BASIC)"
	@echo "$(WHITE)flags:$(BASIC) $(CFLAGS)"
	@echo "$(WHITE)compi:$(BASIC) $(CC)"
	@$(CC) $(CFLAGS) -fvisibility=hidden -rdynamic $(I_DIR) $(O_SRC) $(LIB_UTIL) $(LIBS) -o $@

# $(WARDEN_DUMP): $(O_WARDEN_DUMP) $(LIB_WARDEN)
#	@echo "$(BLUE) $(LIB_WARDEN) $(O_WARDEN_DUMP) $(WHITE)->$(RED) $@ $(BASIC)"
#	@$(CC) $(CFLAGS) $(DEFINES) $(I_DIR) $(LIB_WARDEN) $(O_WARDEN_DUMP) $(LIBS) -o $@


$(O_DIR)/%.o: %.c
	@echo "$(WHITE)$<\t->$(BLUE) $@ $(BASIC)"
	@$(CC) $(CFLAGS)  $(DEFINES) $(I_DIR) -MMD -c -fPIC $< -o $@

$(LIB_UTIL): $(O_UTIL)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(AR) $(LIB_UTIL) $?
	@$(RANLIB) $(LIB_UTIL)

$(LIB_D2GS_MEPH): $(O_D2GS_MEPH)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_DCLONE): $(O_D2GS_DCLONE)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_RECORD): $(O_D2GS_RECORD)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_CHICKEN): $(O_D2GS_CHICKEN)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_PES): $(O_D2GS_PES)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_PICKIT): $(O_D2GS_PICKIT)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_CHEST): $(O_D2GS_CHEST)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_D2GS_PATHING): $(O_D2GS_PATHING)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_MCP_LOGIN): $(O_MCP_LOGIN)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_MCP_GAME): $(O_MCP_GAME)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(LIB_BNCS_LOGIN): $(O_BNCS_LOGIN)
	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

# $(LIB_WARDEN): $(O_WARDEN)
#	@echo "$(BLUE)$?\t$(WHITE)->$(GREEN) $@ $(BASIC)"
# 	@$(CC) $(CFLAGS) $(I_DIR) -shared $? -o $@

$(OBJS): | $(O_DIR)

$(O_DIR):
	@$(MKDIR) $(O_DIR)
	@$(MKDIR) $(PLUGIN_DIR)
	@$(MKDIR) $(CONFIG_DIR)/motoko
	@$(CP) $(ROOT_DIR)/data/motoko.conf.default $(CONFIG_DIR)/motoko/.
	@$(CP) $(ROOT_DIR)/data/plugin.conf.default $(CONFIG_DIR)/motoko/.
	@$(CP) $(ROOT_DIR)/data/wrequest.db.default $(CONFIG_DIR)/motoko/.

install:
	@$(LN) $(HERE)/$(NAME) $(INSTALL_DIR)/$(NAME)

clean:
	@$(RM) $(O_DIR)

fclean: clean
	@$(RM) $(NAME)
	@$(RM) $(PLUGIN_DIR)
#	@$(RM) $(WARDEN_DUMP)

re: fclean all
