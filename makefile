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

#————————————编译子模块————————————#
#方法一：(make -C subdir) 
#方法二：(cd subdir && make)
#加-w 可以查看执行指令时，当前工作目录信息

#————————————编译Exist————————————#
MDK_DIR = ./Micro-Development-Kit

all: $(MDK_DIR)/lib/mdk.a
	(cd Exist && make -w)


#————————————编译mdk————————————#
$(MDK_DIR)/lib/mdk.a:
	@echo "Complie mdk"
	(cd $(MDK_DIR)/mdk_static && make -w)
	@echo ""
	@echo "mdk Complie finished"
	@echo ""
	@echo ""
	@echo ""
	@echo ""

#————————————编译状态服务器————————————#
stateser:
	@echo "Complie mdk"
	(cd $(MDK_DIR)/mdk_static && make -w)
	@echo ""
	@echo "mdk Complie finished"
	@echo ""
	@echo ""
	@echo ""
	@echo ""

