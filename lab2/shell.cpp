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
std::vector<std::string> split(std::string s, const std::string &delimiter);
//int BuildRedirCmd(std::vector<std::string> &args);
int BuildPipeCmd(std::string &cmd);
int BuildInnerCmd(std::vector<std::string> &args);
void SignalHandle(int signum){
	std::cout << std::endl;
	std::cout << "$ ";
	std::cout.flush(); // 清空缓冲
}

bool background;
std::vector<pid_t> bg_process;
char* currentPath = NULL;
char lastPath[256];
std::vector<std::string> history_cmd;
int main(){
	
	std::ios::sync_with_stdio(false);
	std::string cmd;
	signal(SIGINT, SignalHandle); //忽略ctrl+c信号
	//std::cout << getpid() << std::endl;
	std::string historyPath = getenv("HOME");
	historyPath = historyPath + "/.bash_history";
	
	std::ifstream rf_his;
	rf_his.open(historyPath.c_str());
	std::string his;
	if(rf_his.is_open()){
		std::cout << "open" <<std::endl;
		//size_t line = 0;
		while(getline(rf_his, his)){
			history_cmd.push_back(his);
		//std::cout << history_cmd[line];
			//line++;
		}
	}
	else std::cout << "unable to open file" << std::endl;
	rf_his.close();
	std::strcpy(lastPath, getcwd(NULL, 0));
	while(true){
		
		background = false;
		std::cout << "$ ";
		std::getline(std::cin, cmd);
		std::string his_cmd;
		if(cmd[0] == '!'){
			if(cmd[1] == '!'){
				his_cmd = history_cmd[history_cmd.size() - 1];
				cmd.replace(0,2,his_cmd);
			}
			else{
				size_t i;
				for(i = 1; i < cmd.length();i++ ){
					if(cmd[i] != ' ')
						break;
				}
				size_t k;
				for(k = i; k < cmd.length(); k++){
					if(cmd[k] == ' ')
						break;
				}
				if(i == cmd.size()){
					std::cout << "Error !: invalid number" << std::endl;
					continue;
				}
				std::cout << cmd.substr(i,k-i) << std::endl;
				std::istringstream ss(cmd.substr(i, k - i));
				int num;
				ss >> num;
				his_cmd = history_cmd[num - 1];
				cmd.replace(0, k , his_cmd);
			}
			std::cout << cmd << std::endl;
		}	
		history_cmd.push_back(cmd); // 加入当前指令
		std::vector<std::string> args = split(cmd, " ");
		currentPath = getcwd(NULL, 0);
		//std::cout << lastPath << std::endl;
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
		//std::cout << background << std::endl;
		if(background){
			cmd = cmd.substr(0, j + 1);
		}
		if(args[0] == "exit"){
			if(args.size() == 1){
				return 0;
			}
			int num = std::stoi(args[1]);
			return num;
		}
		size_t innerCmd = BuildInnerCmd(args);
		//strcpy(lastPath , currentPath);
		free(currentPath);
		//std::cout << cmd << std::endl;
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

// Inner command
//char lastPath[256] = {0};
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
	else if(args[0] == "cd"){
		size_t args_num = args.size();
		int cd_ret = 0;
		if(args_num != 1 && args_num != 2){
			std::cout << "Error cd: invalid argument" << std::endl;
			return -1;
		}
		else if(args_num == 1){ // 处理仅有cd的情况
			cd_ret = chdir(getenv("HOME")); // 获取HOME的环境变量，并转入
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
	if(args[0] == "wait"){ // 等待所有后台进程

		for(size_t k = 0; k < bg_process.size(); k++){
			waitpid(bg_process[k], NULL, 0); // 等待后台进程
			std::cout << "pid: " << bg_process[k] << " Done!" << std::endl; 
		}
		for(size_t k = 0; k < bg_process.size(); k++){
			bg_process.pop_back();
		}
		return 1;
	}
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
			for(size_t i = history_cmd.size() - num- 1; i < history_cmd.size(); i++){
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
// 管道命令
int BuildPipeCmd(std::string &cmd){
	std::vector<std::string> pipe_cmd = split(cmd, " | ");
	if(pipe_cmd.empty()){
		return 0;
	}
	size_t cmd_count = pipe_cmd.size();
	// 由于重定向只在最后一部分，所以根据最后一部分判断重定向。

	std::vector<std::string> args = split(pipe_cmd[cmd_count - 1], " ");
	
	size_t args_len = args.size(); 
	size_t redir = 0;
	size_t inRedir = count(args.begin(), args.end(), "<");
	size_t outRedir = count(args.begin(), args.end(), ">");
	size_t appendRedir = count(args.begin(), args.end(), ">>");

	std::string filename;
	if(inRedir + outRedir + appendRedir > 1){
		std::cout << "Error redir: invalid redir symbol" << std::endl;
		return -1;
	}
	else if(inRedir == 1 || outRedir == 1 || appendRedir ==1){
		filename = args.back();
		args.pop_back();
		args.pop_back();
		args_len -= 2;
		redir = 1;
	}
	pid_t pid1 = fork();
	pid_t p_gid;
	if(pid1 < 0){
		std::cout << "Error fork" << std::endl;
		return -1;
	}
	if(pid1 == 0){
	/*
		std::cout <<"1:pid  " <<  getpid() <<std::endl;
		std::cout <<"1:ppid  " << getppid() << std::endl;
		std::cout << "1:pgrp " << getpgrp() << std::endl;
	*///	signal(SIGINT, SIG_DFL);
		setpgid(pid1,pid1);
		p_gid  = getpgid(getpid());
		//处理无管道
		if(cmd_count == 1){
			int fd[2];
			//size_t args_len = args.size();
			//std::string filename = args.back();
			if(inRedir){
				fd[0] = open(filename.c_str(), O_RDONLY);
				dup2(fd[0], STDIN_FILENO);
				if(fd[0] != STDIN_FILENO){
					close(fd[0]);
				}
				//args_len -= 2;
			}
			else if(outRedir){
				fd[1] = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
				dup2(fd[1], STDOUT_FILENO);
				if(fd[1] != STDOUT_FILENO){
					close(fd[1]);
				}
				//args_len -= 2;
			}
			else if(appendRedir){
				fd[1] = open(filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(fd[1], STDOUT_FILENO);
				if(fd[1] != STDOUT_FILENO){
					close(fd[1]);
				}
				//args_len -= 2;
			}

			char *args_ptr[args_len + 1];
			for(size_t i = 0 ; i < args_len ; i++){
				args_ptr[i] = &args[i][0];
			}
			args_ptr[args_len] = nullptr;
			if(execvp(args[0].c_str(), args_ptr) == -1){
				std::cout << "Error exe" << std::endl;
				return -1;
			}
		}
		else{
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
				int fd;
				if(redir && process == cmd_count - 1){
					if(inRedir){
						fd = open(filename.c_str(), O_RDONLY);
						dup2(fd, STDIN_FILENO);
						if(fd != STDIN_FILENO){
							close(fd);
						}
					}
					else if(outRedir){
						fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
						dup2(fd, STDOUT_FILENO);
						if(fd != STDOUT_FILENO){
							close(fd);
						}
					}
					else if(appendRedir){
						fd = open(filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
						dup2(fd, STDOUT_FILENO);
						if(fd != STDOUT_FILENO){
							close(fd);
						}
					}
					char *args_ptr[args_len + 1];
					for(size_t i = 0 ; i < args_len ; i++){
						args_ptr[i] = &args[i][0];
					}
					args_ptr[args_len] = nullptr;
					if(execvp(args[0].c_str(), args_ptr) == -1){
						std::cout << "Error exe" << std::endl;
						return -1;
					}
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
                			return -1;
            				}
				}
			}
			else{

		std::cout <<"2 :  pid   " <<  getpid() <<std::endl;
		std::cout << "2: ppid  " << getppid() << std::endl;
		std::cout << "2: pgrp  " << getpgrp() <<std::endl;
				for(size_t j = 0; j < pipe_cmd.size() - 1; j++){
					close(fd[j][0]);
					close(fd[j][1]);
				}
				while(wait(NULL) > 0);
			}
		}
	}
	else{
		setpgid(pid1,pid1);      //独立进程组
		p_gid = getpgid(getpid());
		signal(SIGTTOU, SIG_IGN); //忽略
		//signal(SIGINT, 
		
		tcsetpgrp(0,pid1);       //设为前台进程
		kill(pid1, SIGCONT);     //恢复运行
		if(background){
			waitpid(pid1, NULL, WNOHANG);
			bg_process.push_back(pid1);
			std::cout << pid1 << std::endl;
		}
		else
			waitpid(pid1, NULL, 0);
		//while(wait(NULL) > 0);
		tcsetpgrp(0, p_gid);
	}
	return 0;
} 














