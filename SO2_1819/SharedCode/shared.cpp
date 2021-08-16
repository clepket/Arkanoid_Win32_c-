#include "shared.h"
#include "info.h"

namespace shared {

	MemoryS servMemory;
	MemoryS gameMemory;

	HANDLE gameEvent;
	HANDLE readEvent;

	

	int test;

	Info(*pMemory)[BUFFER][BUFFERSIZE];
	GameInfo *pGameMemory;

	bool InitSharedMemory() {
		test = 0;
		servMemory.hPage = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			PAGESIZE,
			TEXT(PAGE_NAME)
		);
		
		if (servMemory.hPage == NULL)
			return FALSE;

		pMemory = (Info(*)[BUFFER][BUFFERSIZE])MapViewOfFile(servMemory.hPage
			, FILE_MAP_WRITE, 0, 0, sizeof(Info[BUFFER][BUFFERSIZE]));

		if (pMemory == NULL)
			return FALSE;

		servMemory.posRead = 0;
		servMemory.posWrite = 0;

		gameMemory.hPage = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			PAGESIZE,
			TEXT(PAGE_GAME)
		);

		if (gameMemory.hPage == NULL)
			return FALSE;

		pGameMemory = (GameInfo(*))MapViewOfFile(gameMemory.hPage
			, FILE_MAP_WRITE, 0, 0, sizeof(GameInfo));


		if (pGameMemory == NULL)
			return FALSE;


		return TRUE;
	}

	bool OpenSharedMemory() {
		test = 1;
		servMemory.hPage = OpenFileMapping(FILE_MAP_WRITE, TRUE, TEXT(PAGE_NAME));
		
		if (servMemory.hPage == NULL)
			return FALSE;

		pMemory = (Info(*)[BUFFER][BUFFERSIZE])MapViewOfFile(servMemory.hPage
			, FILE_MAP_WRITE, 0, 0, sizeof(Info[BUFFER][BUFFERSIZE]));


		if (pMemory == NULL)
			return FALSE;

		gameMemory.hPage = OpenFileMapping(FILE_MAP_READ, TRUE, TEXT(PAGE_GAME));

		if (gameMemory.hPage == NULL)
			return FALSE;
		else
			printf("helllooooooo\n");

		pGameMemory = (GameInfo(*))MapViewOfFile(gameMemory.hPage
			, FILE_MAP_READ, 0, 0, sizeof(GameInfo));


		if (pGameMemory == NULL)
			return FALSE;
		
		return TRUE;
	}


	bool InitSync() {
		servMemory.hWriteSemaphore = CreateSemaphore(NULL, 1000, 1000, writeSemaphore);
		if (servMemory.hWriteSemaphore == NULL) {
			_tprintf(TEXT("O semaforo de leitura falhou\n"));
			return FALSE;
		}

		servMemory.hReadSemaphore = CreateSemaphore(NULL, 0, 1, readSemaphore);
		if (servMemory.hReadSemaphore == NULL) {
			_tprintf(TEXT("O semaforo de leitura falhou\n"));
			return FALSE;
		}

		servMemory.hReadMutex = CreateMutex(NULL, TRUE, readMutex);
		if (servMemory.hReadMutex == NULL) {
			_tprintf(TEXT("O mutex de leitura falhou\n"));
			return FALSE;
		}

		servMemory.hWriteMutex = CreateMutex(NULL, FALSE, writeMutex);
		if (servMemory.hWriteMutex == NULL) {
			_tprintf(TEXT("O mutex de escrita falhou\n"));
			return FALSE;
		}

		gameEvent = CreateEvent(NULL, TRUE, FALSE, gameSync);

		if (gameEvent == NULL) {
			_tprintf(TEXT("O evento do jogo falhou\n"));
			return FALSE;
		}
		readEvent = CreateEvent(NULL, TRUE, FALSE, readSync);

		if (readEvent == NULL) {
			_tprintf(TEXT("O evento do jogo falhou\n"));
			return FALSE;
		}
		

		return TRUE;
	}

	bool OpenSync() {
		servMemory.hWriteSemaphore = OpenSemaphore(SYNCHRONIZE, FALSE, writeSemaphore);
		if (servMemory.hWriteSemaphore == NULL) {
			_tprintf(TEXT("O semaforo de escrita falhou\n"));
			return FALSE;
		}

		servMemory.hReadSemaphore = OpenSemaphore(SYNCHRONIZE, FALSE, readSemaphore);
		if (servMemory.hReadSemaphore == NULL) {
			_tprintf(TEXT("O semaforo de leitura falhou\n"));
			return FALSE;
		}

		servMemory.hReadMutex = OpenMutex(SYNCHRONIZE, FALSE, readMutex);
		if (servMemory.hReadMutex == NULL) {
			_tprintf(TEXT("O mutex de leitura falhou\n"));
			return FALSE;
		}

		servMemory.hWriteMutex = OpenMutex(SYNCHRONIZE, FALSE, writeMutex);
		if (servMemory.hWriteMutex == NULL) {
			_tprintf(TEXT("O mutex de escrita falhou\n"));
			return FALSE;
		}

		gameEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, gameSync);
		if (gameEvent == NULL) {
			_tprintf(TEXT("O evento do jogo falhou\n"));
			return FALSE;
		}
		readEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, readSync);
		if (readEvent == NULL) {
			_tprintf(TEXT("O evento do jogo falhou\n"));
			return FALSE;
		}

		return TRUE;
	}

	void ReadMemory(Info *info) {
		
		WaitForSingleObject(servMemory.hReadSemaphore, INFINITE);
		
		servMemory.pRead = servMemory.posRead;
		servMemory.posRead = (servMemory.posRead + 1) % BUFFER;

		CopyMemory(info, (*pMemory)[servMemory.pRead], sizeof(Info));


		ReleaseSemaphore(servMemory.hWriteSemaphore, 1, NULL);

	}

	void ReadMemoryServer(Info *info) {

		WaitForSingleObject(readEvent, INFINITE);
		servMemory.pRead = 0;// servMemory.posRead;
		servMemory.posRead = (servMemory.posRead + 1) % BUFFER;

		CopyMemory(info, (*pMemory)[servMemory.pRead], sizeof(Info));
		//ReleaseSemaphore(servMemory.hWriteSemaphore, 1, NULL);
	}


	void ReadGameMemory(GameInfo *gameInfo)
	{
		GameInfo gameInfoTemp;

		WaitForSingleObject(gameEvent, INFINITE); 

		CopyMemory(&gameInfoTemp, pGameMemory, sizeof(gameInfoTemp));

		*gameInfo = gameInfoTemp;
	}


	void WriteMemory(Info info) {
		
		WaitForSingleObject(servMemory.hWriteSemaphore, INFINITE);
		WaitForSingleObject(servMemory.hWriteMutex, INFINITE);

		servMemory.pWrite = 0;//servMemory.posWrite;
		servMemory.posWrite = (servMemory.posWrite + 1) % BUFFER;

		CopyMemory( (*pMemory)[servMemory.pWrite], &info, sizeof(Info));


		ReleaseMutex(servMemory.hWriteMutex);
		ReleaseMutex(servMemory.hReadMutex);
		SetEvent(readEvent);
		ResetEvent(readEvent);

		ReleaseSemaphore(servMemory.hReadSemaphore, 1, NULL);

	}
	

	void WriteGameMemory(GameInfo gameInfo) {

		CopyMemory(pGameMemory, &gameInfo, sizeof(gameInfo));

		SetEvent(gameEvent);
		ResetEvent(gameEvent);

	}
}
