#定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)


#链接mysqlclient动态库  必须放在add_executable之前
link_directories(/usr/lib64/mysql)
#指定生成可执行文件  可该执行文件由SRC_LIST指向的目录下所有的源文件编译而成
add_executable(chatserver ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})


#指定可执行文件链接时需要依赖的库文件
target_link_libraries(chatserver muduo_net muduo_base mysqlclient hiredis pthread)

