# irc_week1
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  --num-callers=20 --track-fds=yes \
  --log-file=valgrind.log \
  ./ircserv 6667 abc
# irc_week2
