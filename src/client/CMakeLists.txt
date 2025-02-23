#定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)

#指定生成可执行文件  该执行文件由SRC_LIST指向的目录下所有的源文件编译而成
add_executable(chatclient ${SRC_LIST})

#指定可执行文件链接时需要依赖的库文件
target_link_libraries(chatclient pthread)