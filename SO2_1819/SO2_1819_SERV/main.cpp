#include<Windows.h>
#include<io.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <thread>
#include <fcntl.h>
#include <tchar.h>
#include <stdlib.h>
#include "shared.h"
#include "main.h"

using namespace shared;

HighScores hs;
BOOL isShutdown;
GameInfo masterGameInfo;


User users[MAX_USERS];

TCHAR input[25];
Info info[25];

int giveId;
int level = 1;


DWORD BallThreadID[MAX_BALLS];
HANDLE BallThreadHandle[MAX_BALLS];
DWORD PrizeThreadID[MAX_PRIZES];
HANDLE PrizeThreadHandle[MAX_PRIZES];
DWORD BlockThreadID[MAX_BLOCKS];
HANDLE BlockThreadHandle[MAX_BLOCKS];


HANDLE hFirstPlayerEvent;
BOOL isFirstPlayer;

void init_board();
void init_board2();
void init_board3();
BOOL checkBarrierMoveLeft(int index);
BOOL checkBarrierMoveRight(int index);
BOOL checkGameLife();
void checkBalls();
void checkBarriersLife();
BOOL getBarrierIndex(int id);
DWORD WINAPI ballControl(LPVOID p);
int getUserIndex(int id);
void checkLevel();




void actionLogin(Info *info) {


	_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdCanceled);
	for (size_t i = 0; i < MAX_USERS; i++)
	{
		if ((_tcsncmp(users[i].username, TEXT(" "), 1) == 0))
		{
			users[i].login = TRUE;
			_tcscpy_s(users[i].username, sizeof(TCHAR[50]), info->user.username);
			_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdLoginConfirmed);
			info->user = users[i];
			info->user.id = users[i].id;
			break;
		}
	}
}

void actionHighScore(Info *info) {
	info->high = hs;
	_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdHighConfirmed);
}

void actionStart(Info *info) {

	if (masterGameInfo.isGame) {
		_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdStartConfirmed);
		for (size_t i = 0; i < MAX_USERS; i++)
		{
			if (masterGameInfo.barrier[i].id == -1) {
				masterGameInfo.barrier[i].id = info->user.id;
				masterGameInfo.barrier[i].lifes = 3;
				masterGameInfo.barrier[i].coord.Y = LOWER_LIMIT;
				masterGameInfo.barrier[i].coord.X = LEFT_LIMIT;
				users[getUserIndex(info->user.id)].score = 0;
				break;
			}
		}
		if (isFirstPlayer) {
			isFirstPlayer = FALSE;
			SetEvent(hFirstPlayerEvent);
		}
	}
	else {

		_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdCanceled);
	}
}

void actionMove(Info *info) {
	int index = getBarrierIndex(info->user.id);
	if (index == -1) {
		_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdSkip);
		return;
	}
	if (_tcsncmp(info->comand, cmdMoveLeft, 9) == 0 && checkBarrierMoveLeft(index)) {
		masterGameInfo.barrier[index].coord.X -= masterGameInfo.barrier[index].speed;
	}
	else if (_tcsncmp(info->comand, cmdMoveRight, 10) == 0 && checkBarrierMoveRight(index)) {
		masterGameInfo.barrier[index].coord.X += masterGameInfo.barrier[index].speed;
	}
	_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdSkip);
}

void TakeAction(Info *info) {
	if (_tcsncmp(info->comand, cmdGameLogin, 5) == 0)
	{
		actionLogin(info);
	}
	else if (_tcsncmp(info->comand, cmdHighScore, 9) == 0)
	{
		actionHighScore(info);
	}
	else if (_tcsncmp(info->comand, cmdStart, 5) == 0)
	{
		actionStart(info);
	}
	else if (_tcsncmp(info->comand, cmdMoveLeft, 9) == 0 || _tcsncmp(info->comand, cmdMoveRight, 10) == 0)
	{
		actionMove(info);
	}
	else {
		_tcscpy_s(info->comand, sizeof(TCHAR[50]), cmdSkip);
	}
}

void setUsersInfo() {
	for (int i = 0; i < MAX_USERS; i++)
	{
		_tcscpy_s(masterGameInfo.info.high.users[i].username, sizeof(TCHAR[50]), users[i].username);
		masterGameInfo.info.high.users[i].id = users[i].id;
		masterGameInfo.info.high.users[i].lifes = users[i].lifes;
		masterGameInfo.info.high.users[i].score = users[i].score;

	}
}


DWORD WINAPI LobbyThread(LPVOID param) {
	Info info;
	GameInfo gameInfo;


	do {
		ZeroMemory(&info, sizeof(info));
		ZeroMemory(&gameInfo, sizeof(gameInfo));

		ReadMemoryServer(&info);
		TakeAction(&info);
		if (_tcsncmp(info.comand, cmdSkip, 4) != 0)
		{
			gameInfo = masterGameInfo;
			gameInfo.info = info;

			if (_tcsncmp(info.comand, cmdLoginConfirmed, 14) == 0)
			{
				gameInfo.info.user.id = users[giveId - 1].id = ++giveId;
				gameInfo.info.user.lifes = users[giveId - 1].lifes = 3;
				gameInfo.info.user.score = users[giveId - 1].score = 0;
			}
			WriteGameMemory(gameInfo);
		}
	} while (isShutdown);
	return 0;
}


DWORD WINAPI GameThread(LPVOID param) {
	_tprintf(TEXT("\nGame Thread"));
	INT_PTR ballId = 0;
	GameInfo dummy;

	masterGameInfo.isGame = TRUE;
	init_board();
	WaitForSingleObject(hFirstPlayerEvent, INFINITE);

	BallThreadHandle[0] = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ballControl,       // thread function name
		reinterpret_cast<LPVOID>(ballId),          // argument to thread function 
		0,                      // use default creation flags 
		&BallThreadID[0]);   // returns the thread identifier 


	do {
		Sleep(0016);
		setUsersInfo();
		checkBarriersLife();
		checkBalls();
		checkLevel();
		dummy = masterGameInfo;
		WriteGameMemory(dummy);
	} while (isShutdown && checkGameLife() && masterGameInfo.isGame);
	masterGameInfo.isGame = FALSE;
	WaitForSingleObject(BallThreadHandle, INFINITE);
	WriteGameMemory(masterGameInfo);
	return 0;
}

/*void createReg() {
	RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &handleKey, NULL);

	if (RegCloseKey(handleKey) == ERROR_SUCCESS) {
		_tprintf(TEXT("\nKey closed successfully\n"));
	}x
	else
		_tprintf(TEXT("\nError closing key\n"));
}*/

void writeReg() {
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &handleKey, NULL) == ERROR_SUCCESS)
		_tprintf(TEXT("\nKey opened successfully\n"));
	else
		_tprintf(TEXT("\nError opening key\n"));

	if (RegSetValueEx(handleKey, v, 0, REG_BINARY, (LPBYTE)& hs, sizeof(HighScores)) == ERROR_SUCCESS)
		_tprintf(TEXT("\nWriting to registry\n"));
	else
		_tprintf(TEXT("\nError writing\n"));

	if (RegCloseKey(handleKey))
		_tprintf(TEXT("\nKey closed successfully\n"));
}

void readReg() {
	if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPWSTR)REGKEY, 0, KEY_ALL_ACCESS, &handleKey) == ERROR_SUCCESS)
		_tprintf(TEXT("\nSuccess\n"));
	else
		_tprintf(TEXT("\nError\n"));

	DWORD size = sizeof(HighScores);

	RegGetValue(HKEY_CURRENT_USER, REGKEY, v, RRF_RT_ANY, NULL, (PVOID)& hs, &size);

	if (RegCloseKey(handleKey) == ERROR_SUCCESS)
		_tprintf(TEXT("\nKey closed successfully\n"));
	else
		_tprintf(TEXT("\nError closing key\n"));

}


void saveScore(int id) {
	int i = getUserIndex(id);

	bool insert = false;
	int p = 0;

	for (int j = 0; j < MAX_REG; j++) {
		if (users[i].score > hs.users[j].score) {
			p = j;
			insert = true;
			break;
		}
	}

	if (insert) {
		HighScores aux;

		for (int j = 0, k = 0; j < MAX_REG; j++) {
			if (j == p) {
				aux.users[j] = users[i];
			}
			else {
				aux.users[j] = hs.users[k];
				k++;
			}
		}

		hs = aux;

	}

	writeReg();


}

void initReg() {
	swprintf_s(hs.users[0].username, TEXT("Diogo"));
	swprintf_s(hs.users[1].username, TEXT("Carlos"));
	swprintf_s(hs.users[2].username, TEXT("André"));
	swprintf_s(hs.users[3].username, TEXT("José"));
	swprintf_s(hs.users[4].username, TEXT("António"));
	swprintf_s(hs.users[5].username, TEXT("João"));
	swprintf_s(hs.users[6].username, TEXT("Bruno"));
	swprintf_s(hs.users[7].username, TEXT("Afonso"));
	swprintf_s(hs.users[8].username, TEXT("Dinis"));
	swprintf_s(hs.users[9].username, TEXT("Tiago"));

	writeReg();
	ZeroMemory(&hs, sizeof(HighScores));
	readReg();

	for (int i = 0; i < 10; i++) {
		_tprintf(TEXT("%d - %s\n"), i + 1, hs.users[i].username);
	}
}

void initBalls() {
	//Initialize ball
	for (int i = 0; i < MAX_BALLS; i++) {
		masterGameInfo.ball[i].isBall = FALSE;
		masterGameInfo.ball[i].coord.X = LEFT_LIMIT + (rand() % RIGHT_LIMIT);
		masterGameInfo.ball[i].coord.Y = LOWER_LIMIT - 20;
		masterGameInfo.ball[i].up = TRUE;
		masterGameInfo.ball[i].speed = BALL_SPEED;
		masterGameInfo.ball[i].inc = BALL_SPEED * 0.2;
		int r = rand() % 2;
		if (r == 0) {
			masterGameInfo.ball[i].right = TRUE;
		}
		else
			masterGameInfo.ball[i].right = FALSE;
		masterGameInfo.ball[i].speed = BALL_SPEED;
		masterGameInfo.ball[i].radius = BALL_RADIUS;
	}
}

void initSoloBall(int id) {
	masterGameInfo.ball[id].coord.X = LEFT_LIMIT + (rand() % RIGHT_LIMIT);
	masterGameInfo.ball[id].coord.Y = LOWER_LIMIT - 20;
	masterGameInfo.ball[id].up = TRUE;
	masterGameInfo.ball[id].speed = BALL_SPEED;
	masterGameInfo.ball[id].inc = BALL_SPEED * 0.2;
	int r = rand() % 2;
	if (r == 0) {
		masterGameInfo.ball[id].right = TRUE;
	}
	else
		masterGameInfo.ball[id].right = FALSE;

	masterGameInfo.ball[id].speed = BALL_SPEED;
	masterGameInfo.ball[id].radius = BALL_RADIUS;
}

void initBlock() {
	//Initialize blocks
	for (int i = 0; i < MAX_BLOCKS; i++) {
		masterGameInfo.block[i].life = 1;
		masterGameInfo.block[i].coord.X = LEFT_LIMIT + 35 + ((i % MAX_ROW_BLOCK) * (BLOCK_WIDTH + 10));
		masterGameInfo.block[i].coord.Y = UPPER_LIMIT + 20 + ((i / MAX_ROW_BLOCK) * (BLOCK_HEIGHT + 20));

		int r = rand() % 3;
		if (r == 0) {
			masterGameInfo.block[i].tipo = normal;
		}
		else if (r == 1) {
			masterGameInfo.block[i].tipo = resistente;
			masterGameInfo.block[i].life = 2 + rand() % 3;
		}
		else if (r == 2) {
			masterGameInfo.block[i].tipo = magico;
		}
	}
}

void initBlock2() {
	//Initialize blocks
	for (int i = 0; i < MAX_BLOCKS; i++) {
		masterGameInfo.block[i].life = 1;
		masterGameInfo.block[i].coord.X = LEFT_LIMIT + 35 + ((i % MAX_ROW_BLOCK) * (BLOCK_WIDTH + 10));
		masterGameInfo.block[i].coord.Y = UPPER_LIMIT + 20 + ((i / MAX_ROW_BLOCK) * (BLOCK_HEIGHT + 20));

		int r = rand() % 3;
		if (r == 0) {
			masterGameInfo.block[i].tipo = magico;
		}
		else if (r == 1) {
			masterGameInfo.block[i].tipo = resistente;
			masterGameInfo.block[i].life = 2 + rand() % 3;
		}
		else if (r == 2) {
			masterGameInfo.block[i].tipo = magico;
		}
	}
}

void initBarrier() {
	//Initialize barrier
	for (int i = 0; i < MAX_USERS; i++) {
		masterGameInfo.barrier[i].id = -1;
		masterGameInfo.barrier[i].lifes = 0;
		masterGameInfo.barrier[i].size = 100;
		masterGameInfo.barrier[i].coord.X = -20;
		masterGameInfo.barrier[i].coord.Y = -20;
		masterGameInfo.barrier[i].speed = BARRIER_SPEED;
	}
}

void initPrizes() {
	//Initialize prizes
	for (int i = 0; i < MAX_PRIZES; i++) {
		masterGameInfo.prize[i].isActive = false;
		masterGameInfo.prize[i].size = 5 * masterGameInfo.ball[0].radius;
		masterGameInfo.prize[i].duration = 10;
		masterGameInfo.prize[i].coord.X = -10;
		masterGameInfo.prize[i].coord.Y = -10;

		int r = rand() % 4;
		if (r == 0) {
			masterGameInfo.prize[i].tipo = speed_up;
		}
		else if (r == 1) {
			masterGameInfo.prize[i].tipo = slow_down;
		}
		else if (r == 2) {
			masterGameInfo.prize[i].tipo = extra_life;
		}
		else if (r == 3) {
			masterGameInfo.prize[i].tipo = triple;
		}

		masterGameInfo.prize[i].speed = 4;
	}
}

void init_board() {
	srand(time(NULL));
	level++;

	initBalls();

	initBlock();

	initBarrier();

	initPrizes();
}

void init_board2() {
	srand(time(NULL));
	level++;

	initBalls();

	initBlock2();

	initPrizes();
}

void init_board_level() {
	srand(time(NULL));
	level++;

	initBalls();

	initBlock();

	initPrizes();

}

void setNextLevel() {
	if (level == 1)
		init_board();
	else if (level == 2)
		init_board2();
	else if (level == 3)
		init_board_level();
	else if (level == 4)
		init_board_level();
	else
		_tprintf(TEXT("\nNo more levels"));
}


int getBarrierIndex(int id) {
	for (int i = 0; i < MAX_USERS; i++) {
		if (masterGameInfo.barrier[i].id == id) {
			return i;
		}
	}
	return -1;
}

int getFirstBarrier() {
	for (int i = 0; i < MAX_USERS; i++) {
		if (masterGameInfo.barrier[i].id != -1) {
			return i;
		}
	}
	return -1;
}

int getUserIndex(int id) {
	for (int i = 0; i < MAX_USERS; i++) {
		if (users[i].id == id) {
			return i;
		}
	}
	return -1;
}

BOOL checkBarrierMoveRight(int index) {
	if (index == -1) {
		return false;
	}
	int xRight = masterGameInfo.barrier[index].coord.X + masterGameInfo.barrier[index].size;

	if (xRight >= RIGHT_LIMIT) {
		return false;
	}

}

BOOL checkBarrierMoveLeft(int index) {
	if (index == -1) {
		return false;
	}
	int xLeft = masterGameInfo.barrier[index].coord.X;

	if (xLeft <= LEFT_LIMIT) {
		return false;
	}

}

BOOL checkGameLife() {

	for (int i = 0; i < MAX_USERS; i++)
	{
		if (masterGameInfo.barrier[i].lifes > 0) {
			return TRUE;
		}
	}
	return FALSE;
}

void checkLevel() {
	BOOL isBlock = FALSE;

	for (int i = 0; i < MAX_BLOCKS; i++)
	{
		if (masterGameInfo.block[i].life > 0) {
			isBlock = TRUE;
			break;
		}
	}
	if (!isBlock)
	{
		setNextLevel();
	}
}

void checkBarriersLife() {
	for (size_t i = 0; i < MAX_USERS; i++)
	{
		if (masterGameInfo.barrier[i].lifes <= 0 && masterGameInfo.barrier[i].id != -1) {
			saveScore(masterGameInfo.barrier[i].id);
			masterGameInfo.barrier[i].coord.X = -50;
			masterGameInfo.barrier[i].coord.Y = -50;
			masterGameInfo.barrier[i].id = -1;
			continue;
		}
	}
}

void checkBalls() {
	for (size_t i = 0; i < MAX_BALLS; i++)
	{
		if (masterGameInfo.ball[i].isBall) {
			return;
		}
	}
	//BALL RESET
	INT_PTR ballId = 0;
	initBalls();
	Sleep(50);
	BallThreadHandle[0] = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ballControl,       // thread function name
		reinterpret_cast<LPVOID>(ballId),          // argument to thread function 
		0,                      // use default creation flags 
		&BallThreadID[0]);   // returns the thread identifier 


}

DWORD WINAPI prizeControl(LPVOID param) {
	INT_PTR id = reinterpret_cast<INT_PTR>(param);
	masterGameInfo.prize[id].isActive = TRUE;
	int number;
	int sleepTime;

	do {
		masterGameInfo.prize[id].coord.Y += masterGameInfo.prize[id].speed;

		Sleep(50);

		for (int i = 0; i < MAX_USERS; i++) {
			if (masterGameInfo.barrier[i].id != -1
				&& masterGameInfo.prize[id].coord.Y + PRIZE_HEIGHT >= masterGameInfo.barrier[i].coord.Y
				&& masterGameInfo.prize[id].coord.X + masterGameInfo.prize[id].size >= masterGameInfo.barrier[i].coord.X
				&& masterGameInfo.prize[id].coord.X <= masterGameInfo.barrier[i].coord.X + masterGameInfo.barrier[i].size) {

				switch (masterGameInfo.prize[id].tipo) {
				case speed_up:
					for (int j = 0; j < MAX_BALLS; j++) {
						if (masterGameInfo.ball[j].isBall && masterGameInfo.ball[j].speed + masterGameInfo.ball[j].inc > BALL_SPEED * 2) {
							masterGameInfo.prize[id].coord.X = -50;
							masterGameInfo.prize[id].coord.Y = -50;
						}
						else if (masterGameInfo.ball[j].isBall) {

							masterGameInfo.ball[j].speed += masterGameInfo.ball[j].inc;
							masterGameInfo.prize[id].coord.X = -50;
							masterGameInfo.prize[id].coord.Y = -50;
							masterGameInfo.prize[id].isBall[j] = TRUE;
						}
					}
					sleepTime = masterGameInfo.prize[id].duration * 1000;
					Sleep(sleepTime);
					for (int j = 0; j < MAX_BALLS; j++) {
						if (masterGameInfo.prize[id].isBall[j]) {
							masterGameInfo.ball[j].speed -= masterGameInfo.ball[j].inc;
							masterGameInfo.prize[id].isBall[j] = FALSE;
						}
					}
					masterGameInfo.prize[id].isActive = FALSE;
					i = MAX_USERS + 1;
					break;

				case slow_down:
					for (int j = 0; j < MAX_BALLS; j++) {
						if (masterGameInfo.ball[j].isBall && masterGameInfo.ball[j].speed - masterGameInfo.ball[j].inc > BALL_SPEED * 0.6) {
							masterGameInfo.prize[id].coord.X = -50;
							masterGameInfo.prize[id].coord.Y = -50;
						}
						else if (masterGameInfo.ball[j].isBall) {
							masterGameInfo.ball[j].speed -= masterGameInfo.ball[j].inc;
							masterGameInfo.prize[id].coord.X = -50;
							masterGameInfo.prize[id].coord.Y = -50;
							masterGameInfo.prize[id].isBall[j] = TRUE;
						}
					}
					sleepTime = masterGameInfo.prize[id].duration * 1000;
					Sleep(sleepTime);
					for (int j = 0; j < MAX_BALLS; j++) {
						if (masterGameInfo.prize[id].isBall[j]) {
							masterGameInfo.ball[j].speed -= masterGameInfo.ball[j].inc;
							masterGameInfo.prize[id].isBall[j] = FALSE;
						}
					}

					masterGameInfo.prize[id].isActive = FALSE;
					i = MAX_USERS + 1;
					break;

				case extra_life:
					masterGameInfo.barrier[i].lifes += 1;
					masterGameInfo.prize[id].coord.X = -50;
					masterGameInfo.prize[id].coord.Y = -50;

					masterGameInfo.prize[id].isActive = FALSE;
					i = MAX_USERS + 1;
					break;

				case triple:
					number = 2;
					for (int k = 0; k < MAX_BALLS; k++) {
						if (!masterGameInfo.ball[k].isBall && number > 0) {
							INT_PTR ballId = k;
							BallThreadHandle[k] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ballControl, reinterpret_cast<LPVOID>(ballId), 0, &BallThreadID[k]);
							number--;
						}
					}

					masterGameInfo.prize[id].coord.X = -50;
					masterGameInfo.prize[id].coord.Y = -50;

					sleepTime = masterGameInfo.prize[id].duration * 1000;
					Sleep(sleepTime);
					int balls = 0;
					for (int k = 0; k < MAX_BALLS; k++) {
						if (masterGameInfo.ball[k].isBall) {
							balls++;
							if (balls > 1) {
								masterGameInfo.ball[k].isBall = FALSE;
								masterGameInfo.ball[k].coord.X = -100;
								masterGameInfo.ball[k].coord.Y = -100;
							}
						}
					}

					masterGameInfo.prize[id].isActive = FALSE;
					i = MAX_USERS + 1;
					break;
				}
			}
		}

		if (masterGameInfo.prize[id].coord.Y > LOWER_LIMIT) {
			masterGameInfo.prize[id].isActive = FALSE;
			masterGameInfo.prize[id].coord.X = -50;
			masterGameInfo.prize[id].coord.Y = -50;
		}

	} while (masterGameInfo.prize[id].isActive);

	return 0;
}

DWORD WINAPI ballControl(LPVOID param) {
	INT_PTR id = reinterpret_cast<INT_PTR>(param);

	initSoloBall(id);

	masterGameInfo.ball[id].isBall = TRUE;
	masterGameInfo.ball[id].user = masterGameInfo.barrier[getFirstBarrier()].id;

	do {
		//Movement
		if (masterGameInfo.ball[id].up) {
			if (masterGameInfo.ball[id].right) {
				masterGameInfo.ball[id].coord.X += masterGameInfo.ball[id].speed;
				masterGameInfo.ball[id].coord.Y -= masterGameInfo.ball[id].speed;
			}
			else {
				masterGameInfo.ball[id].coord.X -= masterGameInfo.ball[id].speed;
				masterGameInfo.ball[id].coord.Y -= masterGameInfo.ball[id].speed;
			}
		}
		else {
			if (masterGameInfo.ball[id].right) {
				masterGameInfo.ball[id].coord.X += masterGameInfo.ball[id].speed;
				masterGameInfo.ball[id].coord.Y += masterGameInfo.ball[id].speed;
			}
			else {
				masterGameInfo.ball[id].coord.X -= masterGameInfo.ball[id].speed;
				masterGameInfo.ball[id].coord.Y += masterGameInfo.ball[id].speed;
			}
		}

		//Verify board limits
		if (masterGameInfo.ball[id].coord.Y <= UPPER_LIMIT) {
			masterGameInfo.ball[id].up = false;
		}
		else if (masterGameInfo.ball[id].coord.Y >= LOWER_LIMIT) {
			masterGameInfo.ball[id].isBall = FALSE;
			masterGameInfo.ball[id].coord.Y = -30;
			masterGameInfo.ball[id].coord.X = -30;

			for (size_t i = 0; i < MAX_USERS; i++) {
				if (masterGameInfo.barrier[i].id != -1) {
					masterGameInfo.barrier[i].lifes -= 1;
				}
			}

			for (int i = 0; i < MAX_PRIZES; i++) {
				masterGameInfo.prize[i].isBall[id] = FALSE;
			}
		}

		if (masterGameInfo.ball[id].coord.X <= LEFT_LIMIT) {
			masterGameInfo.ball[id].right = true;
		}
		else if (masterGameInfo.ball->coord.X >= RIGHT_LIMIT) {
			masterGameInfo.ball[id].right = false;
		}

		//Verify if ball contacts with barrier
		for (int i = 0; i < MAX_USERS && masterGameInfo.ball[id].isBall; i++) {
			if (masterGameInfo.barrier[i].id != -1
				&& masterGameInfo.ball[id].coord.Y + masterGameInfo.ball[id].radius / 2 >= (masterGameInfo.barrier[i].coord.Y - BARRIER_HEIGHT)
				&& masterGameInfo.ball[id].coord.X + masterGameInfo.ball[id].radius >= masterGameInfo.barrier[i].coord.X
				&& masterGameInfo.ball[id].coord.X <= masterGameInfo.barrier[i].coord.X + masterGameInfo.barrier[i].size)
			{
				masterGameInfo.ball[id].up = true;
				masterGameInfo.ball[id].user = masterGameInfo.barrier[i].id;
			}
		}

		//Verify if ball hits blocks 
		for (int i = 0; i < MAX_BLOCKS && masterGameInfo.ball[id].isBall; i++) {
			if (masterGameInfo.block[i].life <= 0) {
				continue;
			}
			if (masterGameInfo.ball[id].coord.Y - masterGameInfo.ball[id].radius <= masterGameInfo.block[i].coord.Y + BLOCK_HEIGHT && masterGameInfo.ball[id].coord.Y + masterGameInfo.ball[id].radius >= masterGameInfo.block[i].coord.Y &&
				masterGameInfo.ball[id].coord.X - masterGameInfo.ball[id].radius <= masterGameInfo.block[i].coord.X + BLOCK_WIDTH && masterGameInfo.ball[id].coord.X + masterGameInfo.ball[id].radius >= masterGameInfo.block[i].coord.X) {

				//Verify which side of the block is hit
				if (masterGameInfo.ball[id].up) {
					if (masterGameInfo.ball[id].right) { //Up and Right
						if (masterGameInfo.ball[id].coord.X - masterGameInfo.block[i].coord.X > masterGameInfo.block[i].coord.Y + BLOCK_HEIGHT - masterGameInfo.ball[id].coord.Y) {
							masterGameInfo.ball[id].up = false;
						}
						else {
							masterGameInfo.ball[id].right = false;
						}
					}
					else { //Up and left
						if (masterGameInfo.block[i].coord.X + BLOCK_WIDTH - masterGameInfo.ball[id].coord.X > masterGameInfo.block[i].coord.Y + BLOCK_HEIGHT - masterGameInfo.ball[id].coord.Y) {
							masterGameInfo.ball[id].up = false;
						}
						else {
							masterGameInfo.ball[id].right = true;
						}
					}
				}
				else {
					if (masterGameInfo.ball[id].right) { //Down and Right
						if (masterGameInfo.ball[id].coord.X - masterGameInfo.block[i].coord.X > masterGameInfo.ball[id].coord.Y - masterGameInfo.block[i].coord.Y) {
							masterGameInfo.ball[id].up = true;
						}
						else {
							masterGameInfo.ball[id].right = false;
						}
					}
					else { //Down and Left
						if (masterGameInfo.block[i].coord.X + BLOCK_WIDTH - masterGameInfo.ball[id].coord.X > masterGameInfo.ball[id].coord.Y - masterGameInfo.block[i].coord.Y) {
							masterGameInfo.ball[id].up = true;
						}
						else {
							masterGameInfo.ball[id].right = true;
						}
					}
				}

				//Add points to player
				users[getUserIndex(masterGameInfo.ball[id].user)].score += 10;
				masterGameInfo.info.high.users[getUserIndex(masterGameInfo.ball[id].user)].score += 10;

				masterGameInfo.block[i].life -= 1;

				if (masterGameInfo.block[i].life <= 0 && masterGameInfo.block[i].tipo == magico) {
					masterGameInfo.prize[i].coord.X = masterGameInfo.block[i].coord.X;
					masterGameInfo.prize[i].coord.Y = masterGameInfo.block[i].coord.Y;

					INT_PTR aux = i;

					PrizeThreadHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)prizeControl, reinterpret_cast<LPVOID>(aux), 0, &PrizeThreadID[i]);

					break;
				}
			}
		}
		Sleep(50);
	} while (masterGameInfo.ball[id].isBall && isShutdown && masterGameInfo.isGame);

	return 0;
}

DWORD WINAPI moveBlocks(LPVOID p) {
	INT_PTR id = reinterpret_cast<INT_PTR>(p);
	do {
		for (int i = 0; i < BLOCK_HEIGHT; i++) {
			if (masterGameInfo.block[id].coord.Y + BLOCK_HEIGHT >= LOWER_LIMIT) {
				masterGameInfo.isGame = false;
				break;
			}
			else {
				masterGameInfo.block[id].coord.Y += 1;
				Sleep(200);
				if (level == 2) {
					Sleep(2000);
				}
				else if (level == 3) {
					Sleep(1500);
				}
				else if (level == 4) {
					Sleep(1000);
				}
				else if (level == 5) {
					Sleep(500);
				}
			}
		}
	} while (masterGameInfo.isGame && masterGameInfo.block[id].life > 0);

	return 0;
}

void setVarUsers() {
	giveId = 0;
	isFirstPlayer = TRUE;

	ZeroMemory(&users, sizeof(users));
	for (size_t i = 0; i < MAX_USERS; i++)
	{
		_tcscpy_s(users[i].username, sizeof(TCHAR[50]), TEXT(" "));
		users[i].id = -1;
		users[i].login = FALSE;
		users[i].score = 0;
		users[i].lifes = 3;
	}

}

int _tmain(int argc, char* argv[])
{

	DWORD LobbyThreadID, GameThreadID;
	HANDLE LobbyThreadHandle, GameThreadHandle;

	isShutdown = TRUE;
	masterGameInfo.isGame = FALSE;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	if (!InitSharedMemory())
	{
		_tprintf(TEXT("Erro ao montar memoria partilhada, a sair...\n"));
		return 50;
	}
	else
	{
		_tprintf(TEXT("Memória partilhada ok...\nA iniciar lobby..."));
	}
	if (!InitSync())
	{
		_tprintf(TEXT("Erro ao iniciar sincronização...\n"));
		return 1;
	}
	else
	{
		_tprintf(TEXT("Sincronização ok...\nA iniciar lobby..."));
	}

	//initReg();
	readReg();

	setVarUsers();
	hFirstPlayerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


	LobbyThreadHandle = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		LobbyThread,       // thread function name
		NULL,          // argument to thread function 
		0,                      // use default creation flags 
		&LobbyThreadID);   // returns the thread identifier 



	do {
		//processar teclado
		ZeroMemory(&input, sizeof(input));
		fflush(stdin);
		_tprintf(TEXT("Nome de utilizador: "));
		_fgetts(input, 25, stdin);


		if ((_tcsncmp(input, TEXT("start"), 5)) == 0 && masterGameInfo.isGame == FALSE)
		{
			GameThreadHandle = CreateThread(
				NULL,                   // default security attributes
				0,                      // use default stack size  
				GameThread,       // thread function name
				NULL,          // argument to thread function 
				0,                      // use default creation flags 
				&GameThreadID);   // returns the thread identifier 

		}
		else if ((_tcsncmp(input, TEXT("score"), 5)) == 0)
		{
			for (int i = 0; i < 10; i++) {
				_tprintf(TEXT("%d - %s\n"), i + 1, hs.users[i].username);
			}
		}
		else if ((_tcsncmp(input, TEXT("move block"), 10)) == 0)
		{
			if (masterGameInfo.isGame) {
				//Lancar thread para mexer tijolos
				for (int i = 0; i < MAX_BLOCKS; i++) {
					INT_PTR index = i;
					BlockThreadHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)moveBlocks, reinterpret_cast<LPVOID>(index), 0, &BlockThreadID[i]);
				}


			}
			else {
				_tprintf(TEXT("O jogo precisa de estar a correr!\n"));
			}
		}
	} while (_tcsncmp(input, TEXT("sair"), 4));

	isShutdown = FALSE;
	/*WaitForSingleObject(LobbyThreadHandle, INFINITE);
	/*WaitForSingleObject(GameThreadHandle, INFINITE);*/
}