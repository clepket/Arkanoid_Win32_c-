#include<Windows.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <thread>
#include <fcntl.h>
#include <stdlib.h>
#include "shared.h"
#include "info.h"

using namespace shared;

DWORD GameThreadID;
HANDLE GameThreadHandle;
BOOL isGame;

HANDLE eventoJogo;
TCHAR input[25];
TCHAR username1;
TCHAR username2;
Comando comando;

void printGame(GameInfo gameInfo) {

}

DWORD WINAPI GameThread(LPVOID param) {

	GameInfo gameInfo;

	ZeroMemory(&gameInfo, sizeof(gameInfo));
	isGame = TRUE;

	do {

		ReadGameMemory(&gameInfo);
		printGame(gameInfo);
		isGame = gameInfo.isGame;
	} while (isGame);
	return 0;
}

void controlGame() {
	GameInfo gameInfo;
	do {
		Sleep(10000);
		isGame = FALSE;
	} while (isGame);
}

void setGame() {

	Info info;
	GameInfo gameInfo;

	ZeroMemory(&info, sizeof(Info));
	ZeroMemory(&gameInfo, sizeof(gameInfo));

	_tcscpy_s(info.comand, sizeof(TCHAR[25]), cmdStart);
	WriteMemory(info);
	ReadGameMemory(&gameInfo);
	if (_tcsncmp(gameInfo.info.comand, cmdStartConfirmed, 14) == 0) {
		
		GameThreadHandle = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			GameThread,       // thread function name
			NULL,          // argument to thread function 
			0,                      // use default creation flags 
			&GameThreadID);

		
		controlGame();
		WaitForSingleObject(GameThreadHandle, INFINITE);
	}
}

void getHighScore() {
	
	Info info, testi;
	GameInfo gameInfo, test23;


	ZeroMemory(&info, sizeof(Info));
	ZeroMemory(&gameInfo, sizeof(gameInfo));

	_tcscpy_s(info.comand, sizeof(TCHAR[25]), cmdHighScore);

	
	WriteMemory(info);
	ReadGameMemory(&gameInfo);

	if (_tcsncmp(gameInfo.info.comand, cmdHighConfirmed, 18) == 0) {
		for (int i = 0; i < 10; i++)
		{
			_tprintf(TEXT("%d - %s\n"), (i + 1), gameInfo.info.high.users[i].username);
		}
	}
}

void menu() {

	_tprintf(TEXT("\n\tArkanoid\n"));
	_tprintf(TEXT("1 - New Game\n"));
	_tprintf(TEXT("2 - Registry\n"));
	_tprintf(TEXT("3 - Exit\n"));

	do {

		ZeroMemory(&input, sizeof(input));
		fflush(stdin);
		_tprintf(TEXT("Escolha: "));
		_fgetts(input, 25, stdin);

		if (_tcsncmp(input, TEXT("1"), 1) == 0) {
			setGame();
		}
		else if (_tcsncmp(input, TEXT("2"), 1) == 0) {
			getHighScore();
		}

	} while (_tcsncmp(input, TEXT("3"), 1));
}

void login() {

	Info info;
	GameInfo gameInfo;
	ZeroMemory(&info, sizeof(info));
	ZeroMemory(&gameInfo, sizeof(gameInfo));

	do {
		ZeroMemory(&input, sizeof(input));
		fflush(stdin);
		_tprintf(TEXT("Nome de utilizador: "));
		_fgetts(input, 25, stdin);
		_tcscpy_s(info.user.username, sizeof(TCHAR[25]), input);
		_tcscpy_s(info.comand, sizeof(TCHAR[25]), cmdGameLogin);
		WriteMemory(info);
		ReadGameMemory(&gameInfo);
	} while (_tcsncmp(gameInfo.info.comand, cmdLoginConfirmed, 14));
 }

int _tmain(int argc, char* argv[]){

	if (!OpenSharedMemory()) {
		_tprintf(TEXT("shit went wrong opening shared memory\n"));
	}
	else {
		_tprintf(TEXT("Everything is ok mate\n"));
	}

	if (!OpenSync()) {
		_tprintf(TEXT("shit went wrong opening semaphores\n"));
	}
	else {
		_tprintf(TEXT("Green light mate\n"));
	} 

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	login();

	menu();

	return 32;
}