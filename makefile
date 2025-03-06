# Nome dell'eseguibile
TARGET = main

# Compilatore e flag
CXX = g++
CXXFLAGS = -O2 -std=c++17

# File sorgente e oggetti
SRCS = main.cpp	
OBJS = $(SRCS:.cpp=.o)

LIBS = -lGLEW -lglfw -lGL

# Regola principale per creare l'eseguibile
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Regola per compilare i file oggetto
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regola per pulire i file oggetto e l'eseguibile
clean:
	rm -f $(OBJS) $(TARGET)

all: $(TARGET)

# Regole di utilità
.PHONY: all clean
