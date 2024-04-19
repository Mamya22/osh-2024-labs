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
 std::vector<std::string> split(std::string s, const std::string &delimiter);
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
