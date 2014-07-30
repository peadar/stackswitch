# CXXFLAGS += -g -Wall
OBJS=stacker.o test.o
LIBS = -lpthread
TARGET = stacker

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)
