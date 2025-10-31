# ============ ft_irc Makefile ============
NAME      := ircserv

CXX       := c++
CXXFLAGS  := -Wall -Wextra -Werror -std=c++98
INCDIR    := include
SRCDIR    := src
BUILDDIR  := build

SRCS := \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/Listener.cpp \
	$(SRCDIR)/Client.cpp \
	$(SRCDIR)/Server.cpp \
	$(SRCDIR)/acceptClients.cpp \
	$(SRCDIR)/Parser.cpp \
	$(SRCDIR)/Channel.cpp \
	$(SRCDIR)/CommandInit.cpp \
	$(SRCDIR)/CommandRegistry.cpp \
	$(SRCDIR)/utils.cpp \
	$(SRCDIR)/cmds/cmd_pass.cpp \
	$(SRCDIR)/cmds/cmd_nick.cpp \
	$(SRCDIR)/cmds/cmd_user.cpp \
	$(SRCDIR)/cmds/cmd_ping.cpp \
	$(SRCDIR)/cmds/cmd_join.cpp \
	$(SRCDIR)/cmds/cmd_part.cpp \
	$(SRCDIR)/cmds/cmd_privmsg.cpp \
	$(SRCDIR)/cmds/cmd_kick.cpp \
	$(SRCDIR)/cmds/cmd_invite.cpp \
	$(SRCDIR)/cmds/cmd_topic.cpp \
	$(SRCDIR)/cmds/cmd_mode.cpp \
	$(SRCDIR)/cmds/cmd_quit.cpp \
	$(SRCDIR)/cmds/cmd_dcc.cpp \
	$(SRCDIR)/cmds/cmd_cap.cpp \
	$(SRCDIR)/cmds/cmd_whois.cpp \
	$(SRCDIR)/cmds/cmd_who.cpp \
	$(SRCDIR)/bonus/Bot.cpp \
	$(SRCDIR)/bonus/Dcc.cpp


OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
DEPS := $(OBJS:.o=.d)

LDFLAGS   :=
LDLIBS    :=

.PHONY: all clean fclean re run dirs tree

all: $(NAME)

$(NAME): dirs $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -MMD -MP -c $< -o $@

dirs:
	@mkdir -p $(BUILDDIR)

run: $(NAME)
	./$(NAME) 6667

clean:
	$(RM) -r $(BUILDDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

tree:
	@printf "\n== Project tree ==\n"; \
	find . -maxdepth 3 -type d -name .git -prune -o -print

-include $(DEPS)
