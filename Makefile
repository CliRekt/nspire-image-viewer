EXE = imageviewer
OBJS = main.o

CXX = nspire-g++
LD = nspire-g++
GENZEHN = genzehn

CXXFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS = -lndls

all: $(EXE).tns

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXE).elf: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(EXE).tns: $(EXE).elf
	$(GENZEHN) --input $< --output $@ --name "$(EXE)"

clean:
	rm -f $(OBJS) $(EXE).elf $(EXE).tns
