gcc -std=c99 -Wextra -march=native -Ofast -fomit-frame-pointer -flto -fuse-linker-plugin -fwhole-program $* bbchess.c
#clang -std=c99 -Wextra -march=native -O3 -fomit-frame-pointer -flto bbchess.c

