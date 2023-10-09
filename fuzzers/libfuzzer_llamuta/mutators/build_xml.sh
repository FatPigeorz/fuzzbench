rm xml.so
# remove -DDEBUG to avoid too much output
g++ -shared -fPIC ./xml.cc -o ./xml.so -DDEBUG -static-libstdc++