
project(card)

cmake_minimum_required(VERSION 2.6)

# add dependencies
add_subdirectory(dep)

# add gameserver sources
add_subdirectory(share/protocol)
#add_subdirectory(dbagent)
add_subdirectory(loginserver)
add_subdirectory(lobby)
#add_subdirectory(center)
#add_subdirectory(transfer_tool)

# add test sources
