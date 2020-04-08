CXXFLAGS := -I.

OBJS := src/main.o src/parse.o src/pcapexport.o xbar/etherbone.o xbar/xbar.o xbar/ft60x/fops.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lcsniff-cli: $(OBJS)
	$(CXX) $(OBJS) -o lcsniff-cli
