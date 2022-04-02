SHELL =	/bin/bash
NAME = ircserv
S = src/
I =	inc/
O = obj/
CC = c++
CFLAGS		=	-g
COMP = $(CC) $(CFLAGS) $(SRC) $(HEADER) -o $(NAME)
COMPO = $(CC) $(CFLAGS) -c $< $(HEADER) -o $@
SRC =  $Smain.cpp $Sserver.cpp $Sparsing.cpp
OBJS = $(SRC:$S%.cpp=$O%.o)
RED			=	\033[1;31m
GRN			=	\033[1;32m
NOCOL		=	\033[0m
HEADER	=	-I $I 


all: $(NAME)

$(NAME): start $(OBJS) 
	@echo -e "$(GRN)]100% ==> Success!$(NOCOL)"
	@$(COMP)

$O%.o: 			$S%.cpp
	@[ -d $(O) ] || mkdir -p $(O)
	@echo -e -n "$(GRN)#####$(NOCOL)"
	@$(COMPO)

start:
	@[ -f "ircserv" ] || echo -e -n "$(GRN)[Compiling]: ["

clean:
	@rm -rf $(O)
	@echo -e "$(RED)Delete obj files!"

fclean:			clean
	@echo -e "$(RED)Delete $(NAME)!"
	@rm -f $(NAME)
	@find . -name ".DS_Store" -delete

re:				fclean start all

.PHONY:			all clean fclean re 