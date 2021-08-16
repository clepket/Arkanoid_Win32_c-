#pragma once

#include "info.h"

namespace shared {

	typedef struct {
		int id;
		int tipo;
	} Comando;

	typedef struct {
		int id;
		COORD coord;
		int size;
		int lifes;
		float speed;
	} Barrier;

	typedef struct {
		TCHAR username[50];
		int id;
		int score;
		int lifes;
		bool login;
	} User;

	typedef struct {
		User users[MAX_REG];
	} HighScores;

	typedef struct {
		User user;
		HighScores high;
		TCHAR comand[50];
	} Info;

	typedef struct {
		BOOL isBall;
		int user;
		BOOL up;
		BOOL right;
		float speed;
		float inc;
		int radius;
		COORD coord;
	} Ball;

	enum Block_Type { normal, resistente, magico};
	
	typedef struct {
		COORD coord;
		int dimension;
		Block_Type tipo;
		int life;
	}Block;

	enum Prize_Type { speed_up, slow_down, extra_life, triple};

	typedef struct {
		COORD coord;
		BOOL isActive;
		BOOL isBall[MAX_BALLS];
		int size;
		Prize_Type tipo;
		int speed;
		int duration;
	}Prize;

	typedef struct {
		TCHAR comand[50];
		Barrier barrier[MAX_USERS];
		Ball ball[MAX_BALLS];  // Ver se só existe uma bola
		Block block[MAX_BLOCKS];
		Prize prize[MAX_PRIZES];
		Info info;
		BOOL isGame;
	} GameInfo;

	typedef struct {
		HANDLE hPage = NULL;
		HANDLE hWriteSemaphore = NULL;
		HANDLE hReadSemaphore = NULL;
		HANDLE hReadMutex = NULL;
		HANDLE hWriteMutex = NULL;

		DWORD pRead = 0;
		DWORD posRead = 0;
		DWORD pWrite = 0;
		DWORD posWrite = 0;
	} MemoryS;

	HANDLE testread;

	__declspec(dllexport) void ReadMemoryServer(Info *info);
	
	__declspec(dllexport) bool OpenSharedMemory();
	__declspec(dllexport) bool InitSharedMemory();
	__declspec(dllexport) void ReadMemory(Info *info);
	__declspec(dllexport) void WriteMemory(Info info);
	__declspec(dllexport) void ReadGameMemory(GameInfo *gameInfo);
	__declspec(dllexport) void WriteGameMemory(GameInfo gameInfo);
	__declspec(dllexport) bool InitSync();
	__declspec(dllexport) bool OpenSync();

}
