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
	size_t pipe_num = pipe_cmd.size();
	if(pipe_num == 1){

	}
	// 需要创建 num - 1 个 fd，管道的内容需要传递，所以需要一个中间量,保存每次读端口，方便下一次使用
	else{
		int mid_bridge;
		for(size_t i = 0; i < pipe_num; i++){
			int fd[2];
			pipe(fd);
			pid_t pid = fork();
			if(pid < 0){
				std::cout << "Error pipe: can not create pipe" << std::endl;
				return 3;
			}
			// 对子进程
			if(pid  == 0){
				if(i == 0){
					close(fd[0]);
					dup2(fd[1], STDOUT_FILENO); //第一个子进程传入数据
				    close(fd[1]);
				}
				else if(i == pipe_num - 1){
					close(fd[1]);
					dup2(mid_bridge, STDIN_FILENO);
					close(mid_bridge);
				}
				else{
					dup2(mid_bridge, STDIN_FILENO); //其余子进程读入上一个
					dup2(fd[1], STDOUT_FILENO);
					close(fd[0]);
					close(fd[1]);
					close(mid_bridge);
				}
			}
			// 对父进程
			else{
				mid_bridge = fd[0];
				close(fd[1]);
				close(fd[0]);

			}
		}
		int status;
		while(waitpid(-1,&status, WNOHANG) != -1){
			continue;
		}
		 
	}
	/*
	else if(pipe_num == 2){
		int fd[2];
		int ret = pipe(fd);
		if(ret == -1){
			std::cout <<"Error pipe: can not create pipe"<<std::endl;
			return 4;
		}
		pid_t pid = fork();
		if(pid == 0){
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);

		}
		else if(pid > 0){
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);
		}
	}
	*/
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
