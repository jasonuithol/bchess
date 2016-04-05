#gcc -std=c99 -Wall -pedantic -march=native -Ofast -fomit-frame-pointer -flto -fuse-linker-plugin -fwhole-program -DBUILD_TESTING bbchess.c
#gcc -g -std=c11 -Wall -pedantic -march=native -O2 -fomit-frame-pointer -flto -fuse-linker-plugin -fwhole-program bbchess.c && valgrind --track-origins=yes --leak-check=full ./a.out
#gcc -g -std=c11 -Wall -pedantic bbchess.c && valgrind --track-origins=yes --leak-check=full ./a.out
gcc -g -std=c11 -Wall -pedantic bbchess.c  -DBUILD_TESTING && valgrind --track-origins=yes --leak-check=full ./a.out
#gcc -g -std=c99 -Wall -pedantic -march=native -Og -fomit-frame-pointer -flto -fuse-linker-plugin -fwhole-program -DBUILD_TESTING bbchess.c && valgrind --track-origins=yes --leak-check=full ./a.out
