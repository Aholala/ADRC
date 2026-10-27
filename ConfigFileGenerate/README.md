图形化环境文件一键生成脚本，当前路径终端下 pyinstaller ConfigFileGenerator.spec 即可打包成exe程序使用

MainDate储存模版文件

UserConfig储存用户生成的配置备份，这个文件夹不会被git跟踪

## [使用方法]

"环境文件配置"页面下，用哪个就勾哪个，注意F4和H7的区别

"CMakeList自定义"可以设置项目名称并添加你当前工程额外需要链接的文件夹

"ecf_configList编辑"可以修改各个模块用到的宏定义

点击生成后，会生成三个文件

- CMakeList.txt 生成在你选择的文件夹下
- ecf_configList 生成在当前文件夹下
- CFG_User.json 生成在ConfigFileGenerate/UserConfig中，用于储存你当前工程自定义的这套配置