#pragma once
#ifndef MYAI_INCLUDED
#define MYAI_INCLUDED 

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#define RED 0
#define BLACK 1
#define CHESS_COVER -1
#define CHESS_EMPTY -2
#define COMMAND_NUM 19

using namespace std;
static unsigned hashColor[2];
static unsigned long long  hashTable[16][32];
static unsigned long long rootHashValue;
struct ChessBoard {
	int Board[32];
	int CoverChess[14];
	int initFlipPOS[32];
	int Red_Chess_Num, Black_Chess_Num;
	int NoEatFlip;
	int initFlipCount;
	int remainNum[14] = { 5,2,2,2,2,2,1, 5,2,2,2,2,2,1 };
	int History[4096];
	int HistoryCount;
	int redMaxPiece;
	int redMaxPos;
	int blackMaxPiece;
	int blackMaxPos;
	double darkPieceValue;
	unsigned long long  hashValue;

	ChessBoard() {}
	ChessBoard(const ChessBoard& chessBoard) {
		memcpy(this->Board, chessBoard.Board, 32 * sizeof(int));
		memcpy(this->initFlipPOS, chessBoard.initFlipPOS, 32 * sizeof(int));
		memcpy(this->CoverChess, chessBoard.CoverChess, 14 * sizeof(int));
		this->Red_Chess_Num = chessBoard.Red_Chess_Num;
		this->Black_Chess_Num = chessBoard.Black_Chess_Num;
		this->NoEatFlip = chessBoard.NoEatFlip;
		memcpy(this->History, chessBoard.History, chessBoard.HistoryCount * sizeof(int));
		this->HistoryCount = chessBoard.HistoryCount;
		this->hashValue = chessBoard.hashValue;
		this->initFlipCount = chessBoard.initFlipCount;
		this->darkPieceValue = chessBoard.darkPieceValue;
	}
};
struct Move {
	int moves ;
	int priority ;
	int eat;
	long int historyValue;
};
struct HashEntry {
	unsigned long long  value = 0;
	double historyOffset = 0;
	double score = 0;
	int depth = 0;
	int type = 3; // 0=> exact 1=>alpha 2=>beta
	// unsigned int pos;
	int move;
};
struct FriendChessList {
	int distance;
	int canEat;
	int value;
	int row;
	int col;
	int i;

};
class MyAI
{
	const char* commands_name[COMMAND_NUM] = {
		"protocol_version",
		"name",
		"version",
		"known_command",
		"list_commands",
		"quit",
		"boardsize",
		"reset_board",
		"num_repetition",
		"num_moves_to_draw",
		"move",
		"flip",
		"genmove",
		"game_over",
		"ready",
		"time_settings",
		"time_left",
	"showboard",
		"init_board"
	};
public:
	MyAI(void);
	~MyAI(void);

	// commands
	bool protocol_version(const char* data[], char* response);// 0
	bool name(const char* data[], char* response);// 1
	bool version(const char* data[], char* response);// 2
	bool known_command(const char* data[], char* response);// 3
	bool list_commands(const char* data[], char* response);// 4
	bool quit(const char* data[], char* response);// 5
	bool boardsize(const char* data[], char* response);// 6
	bool reset_board(const char* data[], char* response);// 7
	bool num_repetition(const char* data[], char* response);// 8
	bool num_moves_to_draw(const char* data[], char* response);// 9
	bool move(const char* data[], char* response);// 10
	bool flip(const char* data[], char* response);// 11
	bool genmove(const char* data[], char* response);// 12
	bool game_over(const char* data[], char* response);// 13
	bool ready(const char* data[], char* response);// 14
	bool time_settings(const char* data[], char* response);// 15
	bool time_left(const char* data[], char* response);// 16
	bool showboard(const char* data[], char* response);// 17
	bool init_board(const char* data[], char* response);// 18

private:
	int Color;
	int Red_Time, Black_Time;
	long int refutationTable[2][32][32];
	ChessBoard main_chessboard;
	bool timeIsUp;
	int purn_node_count;
	bool isDominate = false;
	bool oneRound = false;

#ifdef _WIN64
	clock_t begin;
	clock_t lastTime;
#else
	struct timeval begin;
#endif

	// statistics
	int node;
	int hashHit;
	int hashHit2;
	// Utils
	int GetFin(char c);
	int ConvertChessNo(int input);
	bool isTimeUp();

	// Board
	void initBoardState();
	void initBoardState(const char* data[]);
	void generateMove(char move[6]);
	void MakeMove(ChessBoard* chessboard, const int move, const int chess);
	void MakeMove(ChessBoard* chessboard, const char move[6]);
	bool Referee(const int* board, const int Startoint, const int EndPoint, const int color);
	int Expand(const int* board, const int color, vector<Move>& Result);
	int Expand(ChessBoard* chessboard, const int* board, const int color, Move* Result, Move* opResult, int& opCount);
	int Expand(const int* board, const int color, int* Result);
	double Evaluate(const ChessBoard* chessboard, const int legal_move_count, const int color);
	double Evaluate(const ChessBoard* chessboard, const int legal_move_count, const int color, Move* Result, Move* opResult, int& moveCount, int& opMoveCount);
	double Nega_max(ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth);
	double Nega_max_alpha_bet_purning(const ChessBoard chessboard, int* move, int color, const int depth, double alpha, double beta, const int remain_depth);
	double NegaScout_max_alpha_bet_purning(const ChessBoard chessboard, int* move, int color, const int depth, double alpha, double beta, const int remain_depth);
	double NegaScout_max_alpha_bet_purning_Original ( ChessBoard chessboard, HashEntry* transpositionTable, int* move, int color, const int depth, double alpha, double beta, const int remain_depth, Move& material_exchanging);
	bool isDraw(const ChessBoard* chessboard);
	bool isFinish(const ChessBoard* chessboard, int move_count);
	bool checkQuiescentBoard(ChessBoard* chessboard, const int color, Move& material_exchanging);

	// Display
	void Pirnf_Chess(int chess_no, char* Result);
	void Pirnf_Chessboard();

};

#endif

static const double values[14] = {
			  1,180,  6, 18, 90,270,1200,
			  1,180,  6, 18, 90,270,1200
};