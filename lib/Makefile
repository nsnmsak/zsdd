CXX = clang++


CXXFLAGS +=  -O3 -Wall -Wextra -std=c++11 --stdlib=libc++  


APPS =  zsdd


SRC=$(shell ls *.cc)

all :  $(APPS) 



clean :
	rm -f *.o $(APPS)



dep:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MG $(SRC) >makefile.depend


.cc.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<


zsdd : main.o zsdd_manager.o zsdd_vtree.o zsdd_node.o 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS)  $^ -o $@

-include makefile.depend


