cmake_minimum_required(VERSION 3.15)
project(NetworkServer)

set(CMAKE_CXX_STANDARD 14)
set(Boost_USE_STATIC_LIBS OFF)

find_package(Boost REQUIRED COMPONENTS system thread)

include_directories(.)
include_directories(${Boost_INCLUDE_DIR})

add_executable(server
        main.cpp
        ConnectionParticipant.cpp
        ConnectionParticipant.h
        ConnectionRoom.cpp
        ConnectionRoom.h
        ConnectionSession.cpp
        ConnectionSession.h
        ConnectionServer.cpp
        ConnectionServer.h
        DeliverType.h
        ../GameLogic/Game.cpp
        ../GameLogic/Game.h
        ../GameLogic/Player.h
        ../GameLogic/Ticket.cpp
        ../GameLogic/Ticket.h
        ../GameLogic/Dice.cpp
        ../GameLogic/Dice.h
        ../GameLogic/Human_player.cpp
        ../GameLogic/Human_player.h
        ../GameLogic/Column.h
        ../GameLogic/Column.cpp
        ../GameLogic/C_FromUp.h
        ../GameLogic/C_FromUp.cpp
        ../GameLogic/Field.h
        ../GameLogic/F_Number.h
        ../GameLogic/F_Number.cpp
        ../GameLogic/MinMax.h
        ../GameLogic/MinMax.cpp
        ../GameLogic/Straight.h
        ../GameLogic/Straight.cpp
        ../GameLogic/ThreeOfAKind.h
        ../GameLogic/ThreeOfAKind.cpp
        ../GameLogic/Full.h
        ../GameLogic/Full.cpp
        ../GameLogic/Poker.h
        ../GameLogic/Poker.cpp
        ../GameLogic/Yamb.h
        ../GameLogic/Yamb.cpp
        ../GameLogic/C_Free.h
        ../GameLogic/C_Free.cpp
        ../GameLogic/C_FromBottom.h
        ../GameLogic/C_FromBottom.cpp
        ../GameLogic/C_Hand.h
        ../GameLogic/C_Hand.cpp
        ../GameLogic/C_FromMiddle.h
        ../GameLogic/C_FromMiddle.cpp
        ../GameLogic/C_ToMiddle.h
        ../GameLogic/C_ToMiddle.cpp
        ../GameLogic/C_Announcement.h
        ../GameLogic/C_Announcement.cpp
        ../GameLogic/C_Respond.h
        ../GameLogic/C_Respond.cpp
        ../GameLogic/C_Checkout.h
        ../GameLogic/C_Checkout.cpp
        ../GameLogic/C_Maximum.h
        ../GameLogic/C_Maximum.cpp)

target_link_libraries(server ${Boost_LIBRARIES})
