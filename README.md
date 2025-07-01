# Shell
Customization Shell based on Linux with C


---

# gsh - 简易Linux Shell

一个用C实现的轻量级Linux Shell，支持基础命令、管道和重定向功能。

## 功能 Features
- 基础命令执行（ls/cd等）  
- 管道操作 `|`  
- 输入输出重定向 `> < >>`  
- 命令历史记录（方向键导航）  
- 双引号参数支持 `grep ".c"`  

## 使用方法 Usage
### 编译 Build

make


### 运行 Run

./shell


### 使用示例 Examples

# 管道
shell> ls -l | grep "\.c$"

# 重定向
shell> echo "hello" > output.txt

shell> wc -l < input.txt


---

# gsh - Simple Linux Shell

A lightweight Linux shell implemented in C, supporting basic commands, pipes and redirections.

## Features
- Basic command execution  
- Pipe operations `|`  
- I/O redirections `> < >>`  
- Command history (arrow key navigation)  
- Quoted argument support `grep ".c"`  

## Usage
### Build

make


### Run

./shell

### Examples

# Pipe
shell> ls -l | grep "\.c$"

# Redirection
shell> echo "hello" > output.txt
shell> wc -l < input.txt

# Built-ins
shell> help
shell> cd ~/projects


---

