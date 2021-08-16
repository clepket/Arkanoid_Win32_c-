#pragma once


#include<Windows.h>
#include<io.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <thread>
#include <fcntl.h>
#include <tchar.h>
#include <stdlib.h>



#define PAGE_NAME "ARKANOIDSERV"
#define PAGESIZE 1024*1024*8 //Mbyte
#define WRITESEMAPHORE "writeSemaphore"
#define READSEMAPHORE "readSemaphore"
#define PAGE_GAME "ARKANOIDGAME"
#define PAGESIZE 1024*1024*8 //Mbyte
#define WRITEGAMESEMAPHORE "writeGameSemaphore"
#define READSGAMESEMAPHORE "readGameSemaphore"


#define COMAND "comand"

#define BUFFER 10
#define BUFFERSIZE 1
#define CMD_LOGIN 5
#define REGKEY TEXT("ARKANOID")
#define MAX_REG 10
#define MAX_USERS 10



TCHAR writeSemaphore[] = TEXT("writeSemaphore");
TCHAR readSemaphore[] = TEXT("readSemaphore");
TCHAR writeMutex[] = TEXT("writeMutex");
TCHAR readMutex[] = TEXT("readMutex");
TCHAR gameSync[] = TEXT("gameSync");
TCHAR readSync[] = TEXT("greadSync");



TCHAR cmdGameLogin[] = TEXT("login");
TCHAR cmdHighScore[] = TEXT("highScore");
TCHAR cmdLoginConfirmed[] = TEXT("loginConfirmed");
TCHAR cmdHighConfirmed[] = TEXT("highScoreConfirmed");
TCHAR cmdStart[] = TEXT("start");
TCHAR cmdStartConfirmed[] = TEXT("startConfirmed");
TCHAR cmdMoveLeft[] = TEXT("move_left");
TCHAR cmdMoveRight[] = TEXT("move_right");
TCHAR cmdGame[] = TEXT("game");
TCHAR cmdCanceled[] = TEXT("canceled");
TCHAR cmdSkip[] = TEXT("Skip");


HKEY handleKey;
LPCTSTR v = TEXT("High Scores");


//Variaveis configuraveis para o jogo
#define MAX_LIFES 3
#define MAX_BALLS 3
#define MAX_BLOCKS 66
#define MAX_ROW_BLOCK 11
#define MAX_PRIZES 66
#define LEFT_LIMIT 0
#define RIGHT_LIMIT 700
#define UPPER_LIMIT 0
#define LOWER_LIMIT 600
#define BALL_SPEED 4
#define BALL_RADIUS 5
#define BARRIER_SPEED 15

//LARGURA
#define BLOCK_WIDTH 50

//Altura Barreira
#define BARRIER_HEIGHT 10
#define BLOCK_HEIGHT 20
#define PRIZE_HEIGHT 20