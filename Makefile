SRCS=main.cc
TARGET=test.out

CXXFLAGS+=-I. -O0 

all: $(TARGET)
	
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) -lpthread

clean:
	rm -f $(TARGET)
