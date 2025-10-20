siege -c 10 -t 1M http://localhost:8080/index.html

siege -b http://localhost:8080/index.html

valgrind --leak-check=full --track-origins=yes ./webserv webserv.conf