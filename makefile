#取得顶层目录
TOPDIR = $(shell pwd)

EXECS =  TPSIndex_test

.PHONY : everything deps objs clean veryclean rebuild

everything: $(EXECS)

LIBS := -lpthread

CC  = gcc
CXX = g++
LD  = g++

###define archive option
ARCHIVE  = ar


CFLAGS := -Wall -O3 -g

INCDIR = $(TOPDIR)
CFLAGS += -I$(INCDIR) -I$(TOPDIR)/include -I$(TOPDIR)/src
CFLAGS += -L$(INCDIR) -L$(TOPDIR)/lib

CXXFLAGS := $(CFLAGS)
LDFLAGS += -MD -DLINUX -DUSE_LIB -D_DEBUG_LOG -g

#定义其他变量
RM-F := rm -f

	 
TPSIndextest_HEADER := $(wildcard $(TOPDIR)/*.h) \
		    $(wildcard $(TOPDIR)/src/*.h)

TPSIndextest_SOURCE := $(wildcard $(TOPDIR)/*.cpp) \
			$(wildcard $(TOPDIR)/*.c)


OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(TPSIndex_SOURCE)))
DEPS := $(patsubst %.o,%.d,$(OBJS))
MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS))
MISSING_DEPS_SOURCES := $(wildcard $(patsubst %.d,%.c,$(MISSING_DEPS)) \
$(patsubst %.d,%.cpp,$(MISSING_DEPS)))


TestObj := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(TPSIndextest_SOURCE))) 
TestDEPS := $(patsubst %.o,%.d,$(TestObj))

deps : $(DEPS) $(TestDEPS)
objs : $(OBJS) $(TestObj)


clean :
	@$(RM-F) *.o $(OBJS) $(TestObj)
	@$(RM-F) *.d $(DEPS) $(TestDEPS)

veryclean: clean
	@$(RM-F) $(EXECS)

rebuild: veryclean everything

ifneq ($(MISSING_DEPS),)
$(MISSING_DEPS) :
	@$(RM-F) $(patsubst %.d,%.o,$@)
endif

-include $(DEPS) $(TestDEPS)


TPSIndex_test : $(OBJS) $(TestObj)
	$(LD) -o ./bin/mylgger -I$(INCDIR) $(TestObj) $(OBJS) -lpthread -lrt


