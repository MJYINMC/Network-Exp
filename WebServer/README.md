## Getting Started

Welcome to the VS Code Java world. Here is a guideline to help you get started to write Java code in Visual Studio Code.

## Folder Structure

The workspace contains two folders by default, where:

- `src`: the folder to maintain sources
- `lib`: the folder to maintain dependencies

Meanwhile, the compiled output files will be generated in the `bin` folder by default.

> If you want to customize the folder structure, open `.vscode/settings.json` and update the related settings there.

## Dependency Management

The `JAVA PROJECTS` view allows you to manage your dependencies. More details can be found [here](https://github.com/microsoft/vscode-java-dependency#manage-dependencies).

## URL规范
默认的规范：

http://127.0.0.1:port/test.html -> www/public/html/test.html

http://127.0.0.1:port/noimg.html ->www/public/html/noimg.html

http://127.0.0.1:port/logo.jpg -> www/public/media/logo.jpg

http://127.0.0.1:port/test.txt -> www/public/media/test.txt

可以通过`routes.properties`和`config.properties`进行修改。

## 运行说明

`www`文件夹，`routes.properties`和`config.properties`需要与可执行文件在同一个目录下。

## 编译和打包

javac编译，先打包为jar再用exe4j打包为exe，最后用Enigma Virtual Box将exe与jre打包。

## 另外

值得一提的是，主流浏览器会自动请求网站图标。

http://127.0.0.1:6104/favicon.ico

导致产生了两次http会话，建立了两次TCP连接。