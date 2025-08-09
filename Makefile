all:
	gcc main.c -Wall -Wextra -g -fsanitize=address -lglfw -lvulkan -o main
