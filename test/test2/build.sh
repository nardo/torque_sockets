gcc-4.2 -o client client_main.cpp -g -O0 -I../../lib/libtommath -I../../platform_library -I../../lib/libtomcrypt/src/headers -lstdc++ -ltomcrypt -ltommath -L../../lib/libtommath -L../../lib/libtomcrypt

gcc-4.2 -o server server_main.cpp -g -O0 -I../../lib/libtommath -I../../platform_library -I../../lib/libtomcrypt/src/headers -lstdc++ -ltomcrypt -ltommath -L../../lib/libtommath -L../../lib/libtomcrypt
