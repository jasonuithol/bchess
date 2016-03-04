
#gcc -std=c99 -Wextra -Ofast -march=native -fomit-frame-pointer -flto -fprofile-generate -DPROFILING_BUILD bchess.c
#./run.sh
#gcc -std=c99 -Wextra -Ofast -march=native -fomit-frame-pointer -flto -fprofile-use bchess.c

gcc -std=c99 -Wextra -Ofast -march=native -fomit-frame-pointer -flto -fuse-linker-plugin -fwhole-program bchess.c

