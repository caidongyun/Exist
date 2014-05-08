// Console.h: interface for the Console class.
//
//////////////////////////////////////////////////////////////////////

#if !defined CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>
#include "../Micro-Development-Kit/include/mdk/Thread.h"
#include "../Micro-Development-Kit/include/mdk/Lock.h"


/*
 *	控制台类
 *	接受键盘输入，执行指令
 */
class Console
{
public:
	/*
		prompt 命令行提示符，如果为NULL或""，表示不打印任何信息，只是接收输入，执行指令
		cmdMaxSize	指令最大长度
	*/
	Console(char *prompt, int cmdMaxSize);
	virtual ~Console();

	/*
		开始控制台
		method	类成员的指令受理方法
		pObj	method所属类对象
		fun		全局的指令受理方法

		指令受理方法申明原形
		char* ExecuteCommand(std::vector<std::string> *cmd);
		cmd指令+参数列表，与int main(int argc, char **argv)的argv参数顺序相同
	*/
	bool Start( mdk::MethodPointer method, void *pObj );
	bool Start( mdk::FuntionPointer fun );
	void WaitStop();//等待控制台停止
	void Stop();
	void Print( const char* info );//打印信息，先换行
	void Line();//打印线条
private:
	void* RemoteCall InputThread(void*);//指令输入/执行线程
	char* ExecuteCommand(char *cmd);//执行指令
	void PrintBuf();//打印控制台开始前，接受的输入信息
	void* RemoteCall KeyBroadThread(void*);//键盘线程
		
private:
	bool m_bRun;
	std::string m_prompt;//命令行提示符
	mdk::Thread m_keyBroadThread;
	mdk::Thread m_inputThread;//输入线程
	int m_cmdMaxSize;//指令最大长度
	bool m_printInfo;
	std::vector<std::string> m_cmd;
	std::vector<std::string> m_waitPrint;
	
	bool m_isMemberMethod;
	mdk::MethodPointer m_method;
	void *m_pObj;
	mdk::FuntionPointer m_fun;
	mdk::Mutex m_printLock;
	mdk::Mutex m_ctrlLock;	
};

#endif // CONSOLE_H
