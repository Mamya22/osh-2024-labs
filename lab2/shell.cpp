// IO
#include <iostream>
// std::string
#include <string>
// std::vector
#include <vector>
// std::string 转 int
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/wait.h>
#include<algorithm> 
#include <stdlib.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <map>
std::vector<std::string> split(std::string s, const std::string &delimiter);
//处理外部命令
int BuildPipeCmd(std::string &cmd);
int BuildRedirCmd(std::vector<std::string> &args);
int ExePipeCmd(std::vector<std::string> &pipe_cmd, std::vector<std::string> &args, int redir, size_t cmd_count);
//处理内部命令
int BuildInnerCmd(std::vector<std::string> &args);
// 处理cd命令
int BuildCdCmd(std::vector<std::string> &args);

bool background;   //判断是否为后台
// std::vector<pid_t> bg_process; //记录后台进程
char* currentPath = NULL;  //记录当前路径
char lastPath[256];  //记录上次路径
std::vector<std::string> history_cmd; // 记录历史命令
std::map<std::string, std::string> alias_dict;
std::map<pid_t, std::string> bg_process;
//处理Ctrl C
void SignalHandle(int signum){
	std::cout << std::endl;
	std::cout << "\033[0;35m"<< currentPath << " ";
	std::cout << "$\33[0m ";
	std::cout.flush(); // 清空缓冲
}
// 处理EOF，则对输入流中内容进行判断
int HandleEOF(){
	char c;
	if((c = std::cin.peek()) != EOF){
		// ungetc(c,stdin);
		return 0;
	}
	else{
		std::cout << std::endl<< "exit" << std::endl;
		return 1;
	}
	
}
int main(){
	std::ios::sync_with_stdio(false);
    std::string cmd;
    signal(SIGINT, SignalHandle); // 忽略ctrl+c信号
    //获取当前计算机的历史命令记录
 	std::string historyPath = getenv("HOME");
	historyPath = historyPath + "/.bash_history";
	std::ifstream bash_history;
	bash_history.open(historyPath.c_str());
	std::string cmd_ago;
	if(bash_history.is_open()){
		while(getline(bash_history, cmd_ago)){
			history_cmd.push_back(cmd_ago);
		}
	}   
    else 
        std::cout << "unable to open .bash_history file" << std::endl;    
    bash_history.close();
    // 先获取当前路径
    std::strcpy(lastPath, getcwd(NULL, 0));  

    while(true){
		
        background = false;  // 默认为前台

		currentPath = getcwd(NULL, 0);
		std::cout << "\033[0;35m"<< currentPath << " ";
		std::cout << "$\33[0m ";
		int CtrlD = HandleEOF();
		if(CtrlD == 1){
			exit(0);
		}
		std::getline(std::cin, cmd);
		std::string used_cmd;   
        //处理 ! 指令
		if(cmd[0] == '!'){
			if(cmd[1] == '!'){
				used_cmd = history_cmd[history_cmd.size() - 1];
				cmd.replace(0,2,used_cmd);
			}
			else{
                //找到数字
				size_t i;
				for(i = 1; i < cmd.length(); i++){
					if(cmd[i] != ' ')
						break;
				}
				size_t k;
				for(k = i; k < cmd.length(); k++){
					if(cmd[k] == ' ')
						break;
				}
				if(i == cmd.length()){
					std::cout << "Error !: invalid number" << std::endl;
					continue;
				}
			//	std::cout << cmd.substr(i,k-i) << std::endl;
                //转化为数字
				std::istringstream ss(cmd.substr(i, k - i));
				int num;
				ss >> num;
				used_cmd = history_cmd[num - 1];
				cmd.replace(0, k , used_cmd);
			}
            // cmd为实际命令
			std::cout << cmd << std::endl;
		}	
       	history_cmd.push_back(cmd); // 加入当前指令 

        //处理命令
        //std::vector<std::string> args = split(cmd, " ");
		//currentPath = getcwd(NULL, 0);
        // 判断是否为后台命令
    	size_t j;
		for(j = cmd.length() - 1; j >= 0; j--){
			if(cmd[j] == '&'){
				background = true;
			} 
			else if(cmd[j] == ' ')
				continue;	
			if(cmd[j]!=' ' && cmd[j] != '&'){
				break;
			}
		}   
		if(background){
			cmd = cmd.substr(0, j + 1);
		} 
		//把获得的指令替换成真正的指令
		for(auto alias : alias_dict){
			// std::cout << alias.first << "  " << alias.first.size()<<std::endl;
			if(alias.first == cmd){
				cmd = alias.second;
				break;
			}
		}
        // 开始进行指令运行
		std::vector<std::string> args = split(cmd, " ");
        //处理退出命令
		if(args[0] == "exit"){
			if(args.size() == 1){
				return 0;
			}
			int num = std::stoi(args[1]);
			return num;
		}   
		if(args[0] == "alias"){
			if(args.size() == 1){
				for(auto alias : alias_dict){
					std::cout << "alias	  " << alias.first << " = '" << alias.second <<"'" <<std::endl;
				}
				if(alias_dict.size() == 0){
					std::cout << "empty alias" <<std::endl;
				}
				continue;
			}
			std::string join_cmd ="";
			for(size_t i = 1; i < args.size(); i++){
				join_cmd = join_cmd + args[i] + " ";
			}
			size_t index = join_cmd.find("=");
			if(index == std::string::npos){
				std::cout << "Error alias: without '='" <<std::endl;
				continue;
			}
			std::string other_name = join_cmd.substr(0,index);
			// remove_if(other_name.begin(),other_name.end(), isspace);//去除前后空格
			size_t head_index = 0;
			while(head_index < other_name.size()){
				if(other_name[head_index] !=' ')
					break;
				head_index++;
			}
			size_t tail_index = other_name.size() - 1;
			while(tail_index >= 0){
				if(other_name[tail_index] != ' ')
					break;
				tail_index--;
			}
			other_name = other_name.substr(head_index , tail_index - head_index + 1);
			// std::cout <<head_index <<std::endl;
			// std::cout <<tail_index <<std::endl;
			// std::cout << other_name << "  " << other_name.size() <<std::endl;
			join_cmd.substr(index + 1);
			size_t first_comma_index = join_cmd.find("'");
			size_t last_comma_index = join_cmd.find_last_of("'");
			std::string r_cmd = join_cmd.substr(first_comma_index + 1, last_comma_index - first_comma_index -1);
			alias_dict[other_name] = r_cmd;
			continue;
		}
        //处理内部命令
		int innerCmd = BuildInnerCmd(args);  
        free(currentPath); 
        //处理外部命令
        if (innerCmd == 0){
			BuildPipeCmd(cmd);
		}
    }
}
std::vector<std::string> split(std::string s, const std::string &delimiter){
	std::vector<std::string> res;
	size_t pos = 0;
	std::string token;
	while((pos = s.find(delimiter)) != std::string::npos){
		token = s.substr(0, pos);
		res.push_back(token);
		s = s.substr(pos + delimiter.length());
	}
	res.push_back(s);
	return res;
}

int BuildInnerCmd(std::vector<std::string> &args){
	if(args.empty()){
		return 0;
	}
	else if(args[0] == "pwd"){
		if(args.size() != 1){
			std::cout << "Error pwd: invalid argument" << std::endl;
			return -1;
		}
		char *path = NULL;
		path = getcwd(NULL, 0); // 获取当前路径
		if(path == NULL){
			std::cout << "Error pwd: failed to get current path" << std::endl;
			return -1;
		}
		else{
			std::cout << path << std::endl;
			free(path);
			return 1;
		}
	}   
    if(args[0] == "cd"){
        return BuildCdCmd(args);
    }
    // 处理后台等待命令
    if(args[0] == "wait"){
		for(auto pid : bg_process){
			waitpid(pid.first, NULL, 0);
			std::cout << "Done!  pid: " << pid.first << "	cmd: "<< pid.second << std::endl;
		}
		bg_process.clear();
		// for(size_t k = 0; k < bg_process.size(); k++){
		// 	waitpid(bg_process[k], NULL, 0); // 等待后台进程
		// 	std::cout << "pid: " << bg_process[k] << "      Done!" << std::endl; 
		// }
		// for(size_t k = 0; k < bg_process.size(); k++){
		// 	bg_process.pop_back();
		// }
		return 1;
    }
    //处理历史命令
	if(args[0] == "history"){
		if(args.size() > 2){
			std::cout << "Error history: invalid argument" << std::endl;
			return -1;
		}
		if(args.size() == 2){
			int num;
			std::istringstream ss(args[1]);
			ss >> num;
			if(num <=0 || num >(int)history_cmd.size()){ 
				std::cout << "Error history: error history num" << std::endl;
				return -1;
			}
			for(size_t i = history_cmd.size() - num; i < history_cmd.size(); i++){
				std::cout << "	" << i + 1 << "		" << history_cmd[i] << std::endl;
			}
			return 1;
		}
		else{
			for(size_t i = 0; i < history_cmd.size(); i++){
				std::cout << "	" << i + 1 << "		" << history_cmd[i] << std::endl;
			}
			return 1;
		}
	}
	return 0;    
}
// 处理cd命令
int BuildCdCmd(std::vector<std::string> &args){
    size_t args_num = args.size();
	int cd_ret = 0;
	if(args_num != 1 && args_num != 2){
		std::cout << "Error cd: invalid argument" << std::endl;
		return -1;
	}
	else if(args_num == 1){ // 处理仅有cd的情况
		cd_ret = chdir(getenv("HOME")); // 获取HOME的环境变量，并转入
		strcpy(lastPath , currentPath);
	}
	else if(args_num == 2){
		if(args[1][0] == '~'){  // 处理 cd ~
			std::string mainDir = getenv("HOME"); 
			mainDir = args[1].replace(0, 1, mainDir);
			cd_ret = chdir(mainDir.c_str());
		}
		else if(args[1][0] == '$'){ // 处理环境变量
			size_t len = args[1].length();
			if(len == 1){ //仅有$
				std::cout << "Error cd: invalid argument '$'" <<std::endl;
				return -1;
			}
			size_t index = args[1].find_first_of('/'); //找到子目录
			char* envPath;
			if(index == std::string::npos){
				envPath = getenv(args[1].substr(1, len - 1).c_str());
				if(envPath == NULL){
					std::cout << "Error cd: error environment path" << std::endl;
					return -1;
				}
				cd_ret = chdir(envPath);
			}
			else{
				envPath = getenv(args[1].substr(1, index - 1).c_str());
				if(envPath == NULL){
					std::cout << "Error cd: error environment path" << std::endl;
					return -1;
				}
				cd_ret = chdir(envPath);
			}
		}
		else if(args[1][0] == '-'){
			cd_ret = chdir(lastPath);
		}
		else{
			cd_ret = chdir(args[1].c_str());
		}
		strcpy(lastPath , currentPath);
	}
	if(cd_ret == -1){
		std::cout << "Error cd: failed to change directory" << std::endl;
		return -1;
	}
	return 1;
}
// 管道命令
int BuildPipeCmd(std::string &cmd){
	std::vector<std::string> pipe_cmd = split(cmd, " | ");
	if(pipe_cmd.empty()){
		return 0;
	}
	size_t cmd_count = pipe_cmd.size();
	std::vector<std::string> args = split(pipe_cmd[cmd_count - 1], " ");
	
	size_t args_len = args.size(); 
	size_t redir = 0;
	size_t inRedir = count(args.begin(), args.end(), "<");
	size_t textRedir = count(args.begin(), args.end(), "<<<");
	size_t outRedir = count(args.begin(), args.end(), ">");
	size_t appendRedir = count(args.begin(), args.end(), ">>");
   // std::string filename;
    if(inRedir + outRedir + appendRedir + textRedir >= 1){
		redir = 1;
	}
    pid_t pid1 = fork();
	if(pid1 < 0){
		std::cout << "Error fork" << std::endl;
		return -1;
	}
    if(pid1 == 0){
		setpgid(pid1,pid1);
		if(background){
			signal(SIGINT, SIG_IGN);
			signal(SIGTTOU, SIG_IGN);  
			tcsetpgrp(0, getppid()); // 把父进程挂到前台
		}
		else{
            // tcsetpgrp(0, cpid);

			signal(SIGINT, SIG_DFL); //否则捕获信号
			// signal(SIGINT,exitHandle);
			signal(SIGTTOU, SIG_DFL);
		}
		//处理无管道
		if(cmd_count == 1){
            if(redir){
                BuildRedirCmd(args);
            }
            else{
			    char *args_ptr[args_len + 1];
			    for(size_t i = 0 ; i < args_len ; i++){
			    	args_ptr[i] = &args[i][0];
			    }
			    args_ptr[args_len] = nullptr;
			    if(execvp(args[0].c_str(), args_ptr) == -1){
			    	std::cout << "Error exe" << std::endl;
			    	exit(0);
			    }                
            }
        }   
        else{
			ExePipeCmd(pipe_cmd, args, redir, cmd_count);
			exit(0);
        }         
    }
    else{
		setpgid(pid1,pid1);      //独立进程组
		if(background){
			tcsetpgrp(0, getpgrp());
			waitpid(pid1, NULL, WNOHANG);
			bg_process[pid1] = cmd;
			// std::cout << pid1 << std::endl;
		}
		else{
			tcsetpgrp(0, pid1);
		    signal(SIGTTOU, SIG_IGN); 
			waitpid(pid1, NULL, 0);			
		}
		tcsetpgrp(0, getpgrp());        
    }
    return 0;
}
int ExePipeCmd(std::vector<std::string> &pipe_cmd, std::vector<std::string> &args, int redir, size_t cmd_count){
    // 对于多重管道，需要初始化多个管道。
	int fd[cmd_count - 1][2]; // 需要pipe_cmd.size() - 1 个轨道
	size_t i;
	for(i = 0; i < cmd_count - 1; i++){
		if(pipe(fd[i]) == -1){
			std::cout << "Error pipe: failed to create pipe" << std::endl;
			return -1;
		}
	}
	pid_t pid; // 需要创建pipe_cmd.size()个子进程
	size_t process; // 子进程继承父进程
	for(process = 0; process < pipe_cmd.size(); process++){
		pid = fork();
		if(pid == 0){
			break;
		}
	}
	if(pid == 0){
		if(process == 0){ // 第一个子进程只输出
			dup2(fd[0][1], STDOUT_FILENO);
			close(fd[0][0]);
		}
		else if(process == pipe_cmd.size() - 1){
			dup2(fd[pipe_cmd.size() - 2][0], STDIN_FILENO);
			close(fd[pipe_cmd.size() - 2][1]);
		}
		else{
			dup2(fd[process - 1][0], STDIN_FILENO);
			dup2(fd[process][1], STDOUT_FILENO);
			close(fd[process][0]);
			close(fd[process - 1][1]);
		}
		for(size_t j = 0; j < pipe_cmd.size() - 1; j++){
			if(process != 0 && process != pipe_cmd.size()-1){
				if(process == j || process -1 == j){
					continue;
				}
				close(fd[j][0]);
				close(fd[j][1]);
			}
			else{
				if(j == process){
					continue;
				}
				close(fd[j][0]);
				close(fd[j][1]);
			}
		}
		if(redir && process == cmd_count - 1){
			BuildRedirCmd(args);
		}        
        else{
			std::vector<std::string> args_2 = split(pipe_cmd[process], " ");
			char *ptr[args_2.size() + 1];
    			for(size_t i = 0; i < args_2.size(); i++){
				ptr[i] = &args_2[i][0];
			}
			ptr[args_2.size()] = nullptr;
    		if(execvp(args_2[0].c_str(), ptr) == -1){
        		std::cout << "Error pipe" << std::endl;
        		exit(0);
    		}
		}
    }
    else{
		for(size_t j = 0; j < pipe_cmd.size() - 1; j++){
			close(fd[j][0]);
			close(fd[j][1]);
		}
		while(wait(NULL) > 0);                
    }
	return 0;
}
int BuildRedirCmd(std::vector<std::string> &args){
    // 分离出command;
    size_t pos = 0;
    while(args[pos] != ">" && args[pos] != "<" && args[pos]!= ">>" && args[pos]!="<<<"){
        pos++;
    }
    char *args_ptr[pos + 1];
    for(size_t i = 0 ; i < pos ; i++){
		args_ptr[i] = &args[i][0];
	}
	args_ptr[pos] = nullptr; //结尾要为nullptr
    while(pos < args.size()){
        int fd[2];
        if(args[pos] == "<"){
            std::string filename = args[pos + 1];
            fd[0] = open(filename.c_str(), O_RDONLY);
			// std::cout << fd[0] << std::endl;
			dup2(fd[0], STDIN_FILENO);
			if(fd[0] != STDIN_FILENO){
				close(fd[0]);
			}
            pos++;
        }
        else if(args[pos] == ">"){
            std::string filename = args[pos + 1];
			fd[1] = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
			dup2(fd[1], STDOUT_FILENO);
			if(fd[1] != STDOUT_FILENO){
				close(fd[1]);
			}
            pos++;
        }
        else if(args[pos] == ">>"){
            std::string filename = args[pos + 1];
			fd[1] = open(filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
			dup2(fd[1], STDOUT_FILENO);
			if(fd[1] != STDOUT_FILENO){
				close(fd[1]);
			}   
            pos++;         
        }
        else if(args[pos] == "<<<"){
            std::string text = args[pos + 1]; // 获取文本内容
			text = text + "\n";
            //转化为 < 标准输入流
			// std::FILE* tmpf = std::tmpfile();
			// std::fputs(text.c_str(), tmpf);
			// std::rewind(tmpf);
			// fd[0] = open()
			char filename[100] = "test.XXXXXX";
            fd[0] = mkstemp(filename);
			unlink(filename);
			//std::cout << fd[0] << std::endl;
			if(fd[0] == -1){
				std::cout << "Error <<<: Unable to create temp file" << std::endl;
			}
			write(fd[0], text.c_str(), text.length());
			lseek(fd[0], SEEK_SET, 0);
			dup2(fd[0], STDIN_FILENO);
			close(fd[0]);
			pos++;
			//unlink(filename);
        }
        pos++;
    }        
    if(execvp(args[0].c_str(), args_ptr) == -1){
		std::cout << "Error exe" << std::endl;
		return -1;
    }
    return 1;
}