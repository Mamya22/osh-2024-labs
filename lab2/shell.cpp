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
std::vector<std::string> split(std::string s, const std::string &delimiter);
int BuildPipeCmd(std::string &cmd){
	std::vector<std::string> pipe_cmd = split(cmd," | ");
	if(pipe_cmd.empty()){
		return 0;
	}
	size_t pipe_count = pipe_cmd.size();
	if(pipe_count == 1){ // 无管道
		std::vector<std::string> args = split(pipe_cmd[0], " ");
		char *ptr[args.size() + 1];
		for(size_t i = 0; i < args.size(); i++){
			ptr[i] = &args[i][0];
		}
		ptr[args.size()] = nullptr;
		if(execvp(args[0].c_str(), ptr) == -1){
			std::cout << "Error pipe" << std::endl;
			return -1;
		}
	}
	else{  //有管道
		// 对于多重管道，需要初始化多个管道。
		int fd[pipe_cmd.size() - 1][2]; // 需要pipe_cmd.size() - 1 个轨道
		size_t i;
		for(i = 0; i < pipe_cmd.size() - 1; i++){
			if(pipe(fd[i]) == -1){
				std::cout << "Error pipe: failed to create pipe" << std::endl;
				return -1;
			}
		}
		pid_t pid; // 需要创建pipe_cmd.size()个子进程
		size_t process;
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

			std::vector<std::string> args = split(pipe_cmd[process], " ");
			char *ptr[args.size() + 1];
            for(size_t i = 0; i < args.size(); i++){
				ptr[i] = &args[i][0];
			}
			ptr[args.size()] = nullptr;
         	if(execvp(args[0].c_str(), ptr) == -1){
                std::cout << "Error pipe" << std::endl;
                return -1;
            }
		}
		else{
			for(size_t j = 0; j < pipe_cmd.size() - 1; j++){
				close(fd[j][0]);
				close(fd[j][1]);
			}
			while(wait(NULL) > 0);
		}
		/*
		int fd[2];
		if(pipe(fd) == -1){
			std::cout << "Error pipe: failed to create pipe" << std::endl;
			return -1;
		}
		pid_t pid = fork(); // 创建子进程
		if(pid == 0){ //对子进程1
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO); // 输出传递给写端口
			close(fd[1]);
			// 执行 使用execvp函数
			std::vector<std::string> args = split(pipe_cmd[0], " ");
			char *ptr[args.size() + 1];
			for(size_t i = 0; i < args.size(); i++){
				ptr[i] = &args[i][0];
			}
			ptr[args.size()] = nullptr;
			if(execvp(args[0].c_str(), ptr) == -1){
				std::cout << "Error pipe: failed to execute" << std::endl;
				return -2;
			}
		}
		else{
			pid = fork();
			if(pid == 0){
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);
				close(fd[0]);
			    std::vector<std::string> args = split(pipe_cmd[1], " ");
			    char *ptr[args.size() + 1];
				for(size_t i = 0; i < args.size(); i++){
               		 ptr[i] = &args[i][0];
             	}
             	ptr[args.size()] = nullptr;
        	   if(execvp(args[0].c_str(), ptr) == -1){
              	    std::cout << "Error pipe: failed to execute" << std::endl;
                	 return -2;
				}
			}
			else{
				close(fd[0]);
				close(fd[1]);
				while(wait(NULL) > 0);
			}
		}
	*/	
	}
	
	return 0;
}




int BuildInnerCmd(std::vector<std::string> &args){
	if(args.empty()){
		return 0;
	}
	else if(args[0]== "pwd"){
		if(args.size() != 1){
			std::cout << "Error pwd: invalid_argument" << std::endl;
			return 1;
		}
		char *path = NULL;
		path = getcwd(NULL, 0);
		if(path == NULL){
			std::cout << "Error pwd: Fail to get PATH" <<std::endl;
			return 2;
		}
		else{
			std::cout << path << std::endl;
			free(path);
			return 0;
		}
		
	}
	else if(args[0] == "cd"){
		int cd_ret;
		if(args.size() != 1 && args.size() != 2){
			std::cout << "Error cd: invalid_argument" << std::endl;
			return 1;
		}
		else{
			if(args.size() == 1){
				cd_ret = chdir(getenv("HOME"));
			}
			else if(args.size() == 2){
				if(args[1][0] == '~'){
					std::string mainDir = getenv("HOME");
					//mainDir = mainDir + args[1].substr(1);
					mainDir = args[1].replace(0, 1, mainDir);
					cd_ret = chdir(mainDir.c_str());
				}
				else if(args[1][0] == '$'){
					size_t index = args[1].find_first_of('/');
					char* envirPath;
					if(index == std::string::npos){
						if(args[1].length() == 1){
							std::cout << "Error cd: Failed to get environment path" << std::endl;
							return 3;
						}
						else{
		                   	envirPath = getenv(args[1].substr(1, args[1].length()-1).c_str());
							if(envirPath==NULL){
								std::cout << "Error cd: Failed to get environment path" << std::endl;
								return 3;
							}
	                        cd_ret = chdir(envirPath);
						}
					}
					else {
						envirPath = getenv(args[1].substr(1,index).c_str());
	                        if(envirPath==NULL){
								std::cout << "Error cd: Failed to get environment path" << std::endl;
                                return 3;
							}
						cd_ret = chdir(envirPath);
					}
				}
				else
					cd_ret = chdir(args[1].c_str());
			}
			if(cd_ret == -1){
				std::cout << "Error cd: Failed to change directory" << std::endl;
				return 2;
			}
			else 
				return 0;
		}			
	}
	return 0;
}

int main(){
	std::ios::sync_with_stdio(false);
	std::string cmd;
	while(true){
		std::cout << "$ ";
		std::getline(std::cin, cmd);
		std::vector<std::string> args = split(cmd, " ");
		BuildInnerCmd(args);
		BuildPipeCmd(cmd);
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
