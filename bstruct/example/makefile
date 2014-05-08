#makefile文件编译指令
#如果make文件名是makefile，直接使用make就可以编译
#如果make文件名不是makefile，比如test.txt，那么使用make -f test.txt

#------------------------------------------检查操作系统32位64位--------------------------------------------------------
#SYS_BIT=$(shell getconf LONG_BIT)
#SYS_BIT=$(shell getconf WORD_BIT)
SYS_BIT=$(shell getconf LONG_BIT)
ifeq ($(SYS_BIT),32)
	CPU =  -march=i686 
else 
	CPU = 
endif

#------------------------------------------编辑器--------------------------------------------------------

#c++编译工具
CC = g++ 
#apache模块编译安装工具
APXS = apxs 

#------------------------------------------编辑器End--------------------------------------------------------

#------------------------------------------目录--------------------------------------------------------

#依赖目标/文件查找目录
VPATH = $(OBJ_OUTPUT_DIR) 
#输出目录
OBJ_OUTPUT_DIR=./output
OUTPUT_DIR=./output
$(shell mkdir $(OBJ_OUTPUT_DIR))
$(shell mkdir $(OUTPUT_DIR))

BStruct = /mnt/HYLib/bstruct
#.cpp目录
CPP_DIR= 

#.h目录
H_DIR= 

#------------------------------------------目录End--------------------------------------------------------

#------------------------------------------编译选项--------------------------------------------------------

#SO文件编译选项
CFLAGS= -O -g -fPIC -Wall -D_REENTRANT -DUSE_APACHE -DNO_STRING_CIPHER $(CPU) 

#警告级别
WARNING_LEVEL += -O3 

#头文件目录：-I 目录
INCLUDE = -I. -I../include -I$(BStruct)/include  

#库目录，库文件:-L 目录 -库名
#SYSLIB = -lnsl -lc -lm -lpthread -lstdc++ 
LIB =  

#静态库：.a文件路径名
LIB += $(BStruct)/lib/bstruct.a 

#------------------------------------------编译选项End--------------------------------------------------------

#------------------------------------------输出--------------------------------------------------------
#编译产生的程序文件名
OUTPUT = example

#目标文件
OBJ_PRO = $(notdir $(patsubst %.cpp,%.o,$(wildcard *.cpp))) 

#依赖的目标文件
DEPENDENCE = $(OBJ_PRO) 

#连接项目，需要的所有其它源文件或目标文件(带目录)
OBJ = $(addprefix $(OBJ_OUTPUT_DIR)/, $(OBJ_PRO)) 

#------------------------------------------输出End--------------------------------------------------------

#-------------------------------------------编译指令-----------------------------------------------------
#生成EXE
$(OUTPUT_DIR)/$(OUTPUT):$(DEPENDENCE)
	@echo "Complie (OUTPUT_DIR)/$(OUTPUT)"
	@echo ""
	$(CC) -o $@ $(CFLAGS)$(WARNING_LEVEL)$(INCLUDE)$(OBJ)$(LIB)
	@echo ""
	@echo "$(OUTPUT_DIR)/$(OUTPUT) complie finished"
	@echo ""
	@echo ""
	@echo ""
	@echo ""

#------------------------------------------编译Object----------------------------------------------------
$(OBJ_PRO):%.o:%.cpp %.h
	@echo "Complie $(OBJ_OUTPUT_DIR)/$*.o"
	@echo ""
	$(CC) -c -o $(OBJ_OUTPUT_DIR)/$*.o $(CFLAGS)$(WARNING_LEVEL)$(INCLUDE) $*.cpp
	@echo ""
	@echo "$(OBJ_OUTPUT_DIR)/$*.o complie finished"
	@echo ""
	@echo ""
	@echo ""
	@echo ""

#------------------------------------------清理重新编译----------------------------------------------------
clean:
	-rm -f $(OUTPUT_DIR)/$(OUTPUT) $(OBJ_OUTPUT_DIR)/*.o
	
.PHONY: clean

