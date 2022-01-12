#include "float.h"
#include <algorithm>
#include <iostream>
#include "MyAI.h"
#include <assert.h>
#define MAX_DEPTH 3
#define TIME_LIMIT 30
#define WIN 1.0
#define DRAW 0.2
#define LOSE 0.0
#define BONUS 0.3
#define BONUS_MAX_PIECE 8
#define OFFSET (WIN + BONUS)
#pragma warning(disable : 4996)
#define NOEATFLIP_LIMIT 55
#define POSITION_REPETITION_LIMIT 3
#define HISTORY_HERUSTIC_ACTIVE false

#define IDAS_DEPTH 2
#define IDAS_THRESHOLD 0.1
#define RED_DOMINATE 0
#define BLACK_DOMINATE 1

#define HASH_ACTIVE false
#define EXACT_VALUE 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

#define OPENING_TIME 750
#define INTER_TIME  50000
#define ENDGAME_TIME 0

void initTranspositionTable() {
	hashColor[0] = rand();
	hashColor[1] = rand();
	rootHashValue = 0;
	// 14=> cover 15=>empty
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 32; j++)
		{
			hashTable[i][j] = ((unsigned long long)rand() << 32) + rand();
		}
	}
	for (int j = 0; j < 32; j++)
	{
		rootHashValue ^= hashTable[14][j];
	}
}

MyAI::MyAI(void) {
	srand(time(NULL));
	initTranspositionTable();
	main_chessboard.hashValue = rootHashValue;
}

MyAI::~MyAI(void) {}

bool MyAI::protocol_version(const char* data[], char* response) {
	strcpy(response, "1.1.0");
	return 0;
}

bool MyAI::name(const char* data[], char* response) {
	strcpy(response, "MyAI");
	return 0;
}

bool MyAI::version(const char* data[], char* response) {
	strcpy(response, "1.0.0");
	return 0;
}

bool MyAI::known_command(const char* data[], char* response) {
	for (int i = 0; i < COMMAND_NUM; i++) {
		if (!strcmp(data[0], commands_name[i])) {
			strcpy(response, "true");
			return 0;
		}
	}
	strcpy(response, "false");
	return 0;
}

bool MyAI::list_commands(const char* data[], char* response) {
	for (int i = 0; i < COMMAND_NUM; i++) {
		strcat(response, commands_name[i]);
		if (i < COMMAND_NUM - 1) {
			strcat(response, "\n");
		}
	}
	return 0;
}

bool MyAI::quit(const char* data[], char* response) {
	fprintf(stderr, "Bye\n");
	return 0;
}

bool MyAI::boardsize(const char* data[], char* response) {
	fprintf(stderr, "BoardSize: %s x %s\n", data[0], data[1]);
	return 0;
}

bool MyAI::reset_board(const char* data[], char* response) {
	this->Red_Time = -1; // unknown
	this->Black_Time = -1; // unknown
	this->initBoardState();
	return 0;
}

bool MyAI::num_repetition(const char* data[], char* response) {
	return 0;
}

bool MyAI::num_moves_to_draw(const char* data[], char* response) {
	return 0;
}

bool MyAI::move(const char* data[], char* response) {
	char move[6];
	sprintf(move, "%s-%s", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::flip(const char* data[], char* response) {
	char move[6];
	sprintf(move, "%s(%s)", data[0], data[1]);
	this->MakeMove(&(this->main_chessboard), move);
	return 0;
}

bool MyAI::genmove(const char* data[], char* response) {
	// set color
	if (!strcmp(data[0], "red")) {
		this->Color = RED;
	}
	else if (!strcmp(data[0], "black")) {
		this->Color = BLACK;
	}
	else {
		this->Color = 2;
	}
	// genmove
	char move[6];
	this->generateMove(move);
	sprintf(response, "%c%c %c%c", move[0], move[1], move[3], move[4]);
	return 0;
}

bool MyAI::game_over(const char* data[], char* response) {
	fprintf(stderr, "Game Result: %s\n", data[0]);
	return 0;
}

bool MyAI::ready(const char* data[], char* response) {
	return 0;
}

bool MyAI::time_settings(const char* data[], char* response) {
	return 0;
}

bool MyAI::time_left(const char* data[], char* response) {
	if (!strcmp(data[0], "red")) {
		sscanf(data[1], "%d", &(this->Red_Time));
	}
	else {
		sscanf(data[1], "%d", &(this->Black_Time));
	}
	fprintf(stderr, "Time Left(%s): %s\n", data[0], data[1]);
	return 0;
}

bool MyAI::showboard(const char* data[], char* response) {
	Pirnf_Chessboard();
	return 0;
}

bool MyAI::init_board(const char* data[], char* response) {
	initBoardState(data);
	return 0;
}

bool Movecompare(Move& a, Move& b) {
	
	assert(b.priority >= 0);
	assert(a.priority >= 0);
	return a.priority > b.priority; 
}
bool HistoryMovecompare(Move& a, Move& b) {
	return a.historyValue > b.historyValue;
}

// *********************** AI FUNCTION *********************** //

int MyAI::GetFin(char c)
{
	static const char skind[] = { '-','K','G','M','R','N','C','P','X','k','g','m','r','n','c','p' };
	for (int f = 0; f < 16; f++)if (c == skind[f])return f;
	return -1;
}

int MyAI::ConvertChessNo(int input)
{
	switch (input)
	{
	case 0:
		return CHESS_EMPTY;
		break;
	case 8:
		return CHESS_COVER;
		break;
	case 1:
		return 6;
		break;
	case 2:
		return 5;
		break;
	case 3:
		return 4;
		break;
	case 4:
		return 3;
		break;
	case 5:
		return 2;
		break;
	case 6:
		return 1;
		break;
	case 7:
		return 0;
		break;
	case 9:
		return 13;
		break;
	case 10:
		return 12;
		break;
	case 11:
		return 11;
		break;
	case 12:
		return 10;
		break;
	case 13:
		return 9;
		break;
	case 14:
		return 8;
		break;
	case 15:
		return 7;
		break;
	}
	return -1;
}


void MyAI::initBoardState()
{
	int iPieceCount[14] = { 5, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 1 };
	memcpy(main_chessboard.CoverChess, iPieceCount, sizeof(int) * 14);
	main_chessboard.Red_Chess_Num = 16;
	main_chessboard.Black_Chess_Num = 16;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	// convert to my format
	int Index = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			main_chessboard.Board[Index] = CHESS_COVER;
			Index++;
		}
	}
	Pirnf_Chessboard();
}
void MoveOrderSort(const int* board, Move* moves, int& count) {
	sort(moves, moves + count, Movecompare);
	
	for (int i = 0; i < count; i++) {
		for (int k = i; k > 0; k--) {
		

			if (board[moves[k].moves % 100] < 0 && board[moves[k - 1].moves % 100] < 0) {
				// empty step
				if (rand() % 2) {
					swap(moves[k], moves[k - 1]);
				}
			}
			if (board[moves[k].moves % 100] % 7 > board[moves[k - 1].moves % 100] % 7) {
				swap(moves[k], moves[k - 1]);
			}
			else break;
		}
	}
}

void MyAI::initBoardState(const char* data[])
{
	memset(main_chessboard.CoverChess, 0, sizeof(int) * 14);
	main_chessboard.Red_Chess_Num = 0;
	main_chessboard.Black_Chess_Num = 0;
	main_chessboard.NoEatFlip = 0;
	main_chessboard.HistoryCount = 0;

	// set board
	int Index = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			// convert to my format
			int chess = ConvertChessNo(GetFin(data[Index][0]));
			main_chessboard.Board[Index] = chess;
			if (chess != CHESS_COVER && chess != CHESS_EMPTY) {
				main_chessboard.CoverChess[chess]--;
				(chess < 7 ?
					main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num)++;
			}
			Index++;
		}
	}
	// set covered chess
	for (int i = 0; i < 14; i++) {
		main_chessboard.CoverChess[i] += data[32 + i][0] - '0';
		(i < 7 ?
			main_chessboard.Red_Chess_Num : main_chessboard.Black_Chess_Num)
			+= main_chessboard.CoverChess[i];
	}

	Pirnf_Chessboard();
}
bool MyAI::isTimeUp() {
	this->timeIsUp = ((double)(clock() - begin) / CLOCKS_PER_SEC >= TIME_LIMIT);
	return this->timeIsUp && this->oneRound;
}
double MyAI::NegaScout_max_alpha_bet_purning(ChessBoard chessboard, int* move, int color, const int depth, double alpha, double beta, const int remain_depth) {
	vector<Move> Moves, opMoves;

	/*int Chess[2048];*/
	int move_count = 0, flip_count = 0, remain_count = 0, remain_total = 0, opMove_count = 0;
	// move
	//move_count = Expand(&chessboard, chessboard.Board, color, Moves, opMoves, opMove_count);
	// MoveOrderSort(chessboard.Board, Moves, move_count);
	for (int i = 0; i < move_count; i++) {
		int srcPiecePos = Moves[i].moves / 100;
		int dstPiecePos = Moves[i].moves % 100;
		Moves[i].historyValue = refutationTable[chessboard.Board[srcPiecePos] / 7][srcPiecePos][dstPiecePos];
	}
	for (int i = 0; i < move_count; i++) {
		for (int k = i; k > 0; k--) {
			if (Moves[k].historyValue > Moves[k - 1].historyValue) {
				swap(Moves[k], Moves[k - 1]);
				// fprintf(stderr, "[HH] replaced! move: %d -> %d value: %d -> %d\n", Moves[k].moves, Moves[k - 1].moves, Moves[k].historyValue, Moves[k-1].historyValue);
				// fflush(stderr);
			}
			else break;
		}
	}
	//sort(Moves.begin(), Moves.end(), Movecompare);
	//assert(moves.size() == count);
	//for (int i = 0; i < Moves.size(); i++) {
	//	for (int k = i; k > 0; k--) {


	//		if (chessboard.Board[Moves[k].moves % 100] < 0 && chessboard.Board[Moves[k - 1].moves % 100] < 0) {
	//			// empty step
	//			if (rand() % 2) {
	//				swap(Moves[k], Moves[k - 1]);
	//			}
	//		}
	//		if (chessboard.Board[Moves[k].moves % 100] % 7 > chessboard.Board[Moves[k - 1].moves % 100] % 7) {
	//			swap(Moves[k], Moves[k - 1]);
	//		}
	//		else break;
	//	}
	//}
	//// flip
	//for (int j = 0; j < 14; j++) { // find remain chess
	//	if (chessboard.CoverChess[j] > 0) {
	//		Chess[remain_count] = j;
	//		remain_count++;
	//		remain_total += chessboard.CoverChess[j];
	//	}
	//}
	//for (int i = 0; i < 32; i++) { // find cover
	//	if (chessboard.Board[i] == CHESS_COVER) {
	//		Moves[move_count + flip_count].moves = i * 100 + i;
	//		flip_count++;
	//	}
	//}

	// random_shuffle(Moves.begin(), Moves.end());
	if (isTimeUp() || // time is up
		remain_depth <= 0 ||   // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count + flip_count == 0 // terminal node (no move type)
		) {

		this->node++;

		// odd: *-1, even: *1
		return 0;// Evaluate(&chessboard, move_count + flip_count, color, Moves, opMoves)* (depth & 1 ? -1 : 1);
	}
	else {
		double m = -DBL_MAX;
		double n = beta;
		int new_move;
		// search deeper
		for (int i = 0; i < move_count; i++) { // move
			ChessBoard new_chessboard = chessboard;
			// new FriendChessList
			bool flag = false;
			int srcPiecePos = Moves[i].moves / 100;
			int dstPiecePos = Moves[i].moves % 100;
			if (chessboard.HistoryCount > 6) {
				flag = (chessboard.History[chessboard.HistoryCount - 4] == Moves[i].moves) && (chessboard.History[chessboard.HistoryCount - 2] == chessboard.History[chessboard.HistoryCount - 6]);
			}
			if (flag)continue;
			MakeMove(&new_chessboard, Moves[i].moves, 0); // 0: dummy
			double t = -NegaScout_max_alpha_bet_purning(new_chessboard, &new_move, color ^ 1, depth + 1, -n, -max(alpha, m), remain_depth - 1); // Scout testing
			if (t > m) {
				if (abs(n - beta) < 0.00001 || remain_depth < 3 || t >= beta) {
					m = t;
					*move = Moves[i].moves;
				}
				else {
					m = -NegaScout_max_alpha_bet_purning(new_chessboard, &new_move, color ^ 1, depth + 1, -beta, -t, remain_depth - 1); // re-search
					*move = Moves[i].moves;
				}
			}

			if (m >= beta) {
				this->purn_node_count += move_count - i;
				refutationTable[chessboard.Board[srcPiecePos] / 7][srcPiecePos][dstPiecePos] += 1 << remain_depth;
				return m;
			}
			n = max(alpha, m) + 1;

		}

		//for (int i = move_count; i < move_count + flip_count; i++) { // flip
		//	double total = 0;
		//	for (int k = 0; k < remain_count; k++) {
		//		ChessBoard new_chessboard = chessboard;

		//		MakeMove(&new_chessboard, Moves[i], Chess[k]);
		//		double t = -Nega_max_alpha_bet_purning(new_chessboard, &new_move, color ^ 1, depth + 1, -beta, -m, remain_depth - 1);
		//		total += chessboard.CoverChess[Chess[k]] * t;
		//	}

		//	double E_score = (total / remain_total); // calculate the expect value of flip
		//	if (E_score > m) {
		//		m = E_score;
		//		*move = Moves[i];
		//	}
		//	else if (E_score == m) {
		//		bool r = rand() % 2;
		//		if (r) *move = Moves[i];
		//	}
		//}
		int srcPiecePos = *move / 100;
		int dstPiecePos = *move % 100;
		refutationTable[chessboard.Board[srcPiecePos] / 7][srcPiecePos][dstPiecePos] += 1 << remain_depth;
		return m;
	}
}
double MyAI::NegaScout_max_alpha_bet_purning_Original(ChessBoard chessboard, HashEntry* transpositionTable, int* move, int color, const int depth, double alpha, double beta, const int remain_depth, Move& material_exchanging) {

	Move Moves[2048];
	Move opMoves[2048];
	/* handle hash*/
	HashEntry& currentHash = transpositionTable[chessboard.hashValue & 0xfffff]; // equals to % 2^20
	// cout << "chessboard.hashValue & 0xfffff : " << (chessboard.hashValue & 0xfffff) << '\n';
	

	int move_count = 0, flip_count = 0, remain_count = 0, remain_total = 0, opMove_count = 0;
	int index = -1;
	int dominate = -1;
	int Chess[2048];
	double darkPieceValue = 0;
	// flip
	for (int j = 0; j < 14; j++) { // find remain chess
		if (chessboard.CoverChess[j] > 0) {
			darkPieceValue += (this->Color == j / 7) ? chessboard.CoverChess[j] * values[j] : -chessboard.CoverChess[j] * values[j];
			Chess[remain_count] = j;
			remain_count++;
			remain_total += chessboard.CoverChess[j];
		}
	}
	chessboard.darkPieceValue = (remain_total == 0) ? 0 : darkPieceValue / remain_total;
	// move
	move_count = Expand(&chessboard,chessboard.Board, color, Moves, opMoves, opMove_count);
	for (int i = 0; i < move_count; i++) { // move
		if (chessboard.HistoryCount > 6) {

			if ((chessboard.History[chessboard.HistoryCount - 4] == Moves[i].moves) && (chessboard.History[chessboard.HistoryCount - 2] == chessboard.History[chessboard.HistoryCount - 6])) {
				Moves[i] = Moves[move_count - 1];
				
				move_count--;
				break;
			}
		}

	}
	MoveOrderSort(chessboard.Board, Moves, move_count);
	for (int i = 0; i < 32; i++) { // find cover
		if (chessboard.Board[i] == CHESS_COVER) {
			
			Moves[move_count + flip_count].moves = i * 100 + i;
			flip_count++;
		}
	}
	//if (chessboard.initFlipCount > 0 && depth == 0) {
	//	for (int i = 0; i < chessboard.initFlipCount; i++) {
	//		Move temp;
	//		temp.moves =  chessboard.initFlipPOS[i];
	//		Moves.push_back(temp);
	//		flip_count++;
	//	}
	//}
	//else {
	//	for (int i = 0; i < 32; i++) { // find cover
	//		if (chessboard.Board[i] == CHESS_COVER) {
	//			Move temp;
	//			temp.moves = i * 100 + i;
	//			Moves.push_back(temp);

	//			flip_count++;
	//		}
	//	}
	//}
	
	if (flip_count > 26 ) {
		*move = Moves[rand() % flip_count].moves;
		return 0;
	}
	
	/*if (((chessboard.remainNum[0] == 0 && chessboard.remainNum[1] == 0) && chessboard.remainNum[13] > 0) ||
		((chessboard.redMaxPiece < chessboard.blackMaxPiece) && !(chessboard.redMaxPiece == 0 && chessboard.blackMaxPiece == 6))
		) {
		dominate = BLACK_DOMINATE;
		
	}
	else if (((chessboard.remainNum[7] == 0 && chessboard.remainNum[8] == 0) && chessboard.remainNum[6] > 0) ||
		((chessboard.blackMaxPiece < chessboard.redMaxPiece) && !(chessboard.blackMaxPiece == 0 && chessboard.redMaxPiece == 6))
		) {
		dominate = RED_DOMINATE;
	}
	isDominate = (dominate == this->Color);*/
	
	
	if (isTimeUp()||// time is up
		(remain_depth <= 0 && !checkQuiescentBoard(&chessboard, color, material_exchanging)) ||   // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count + flip_count == 0 // terminal node (no move type)
		) {

		this->node++;

		// odd: *-1, even: *1
		return Evaluate(&chessboard, move_count + flip_count, color, Moves, opMoves, move_count, opMove_count) * (depth & 1 ? -1 : 1);
	}
	else {
		double m = -DBL_MAX;
		if (HASH_ACTIVE) {
			if (currentHash.value == chessboard.hashValue && currentHash.type != 3) {
				// hash hit
				// fprintf(stderr, "[HASH HIT]\n");
				// cout << currentHash.value << ' ' << chessboard.hashValue;

				// fflush(stderr);
				if (currentHash.depth >= depth) {
					/*fprintf(stderr, "[HASH CHANGE MOVE]\n");
					fflush(stderr);*/

					if (currentHash.type == EXACT_VALUE) {
						//exact value
						this->hashHit++;
						*move = currentHash.move;
						return currentHash.score ;
					}
					else if(currentHash.type == LOWER_BOUND) {
						
						if (currentHash.score  >= beta) {
							*move = currentHash.move;
							return currentHash.score ;
						}
						alpha = max(alpha, currentHash.score);

					}
					else if (currentHash.type == UPPER_BOUND) {
						if (currentHash.score <= alpha) {
							*move = currentHash.move;
							return currentHash.score ;
						}
						beta = min(beta, currentHash.score );

					}
				}
				else {
					this->hashHit2++;
					if (currentHash.type == EXACT_VALUE) {
						m = currentHash.score ;
						*move = currentHash.move ;
					}
				}
			}
		}
		
		double n = beta;

		int new_move;
		// search deeper
		for (int i = 0; i < move_count + flip_count; i++) { // move
			ChessBoard new_chessboard = chessboard;
			// new FriendChessList
			
			int srcPiecePos = Moves[i].moves / 100;
			int dstPiecePos = Moves[i].moves % 100;
			
			// cout << "[HASH] before move hash : " << new_chessboard.hashValue << "\n";
			MakeMove(&new_chessboard, Moves[i].moves, 0); // 0: dummy
			// cout << "[HASH] after move hash : " << new_chessboard.hashValue << "\n";
			double t = 0;
			if (i < move_count) {
				t = -NegaScout_max_alpha_bet_purning_Original(new_chessboard, transpositionTable, &new_move, color ^ 1, depth + 1, -n, -max(alpha, m), remain_depth - 1, Moves[i]); // Scout testing
			}
			else {
				
					double total = 0;
					for (int k = 0; k < remain_count; k++) {
						
						ChessBoard new_chessboard = chessboard;

						MakeMove(&new_chessboard, Moves[i].moves, Chess[k]);
						double t = -NegaScout_max_alpha_bet_purning_Original(new_chessboard, transpositionTable, &new_move, color ^ 1, depth + 1, -n, -max(alpha, m), remain_depth - 1, Moves[i]); // Scout testing

						total += chessboard.CoverChess[Chess[k]] * t;
					}

					t = (total / remain_total); // calculate the expect value of flip
				
				
			}
			
			assert(new_chessboard.hashValue != chessboard.hashValue);
			if (t > m) {
				if (abs(n - beta) < 0.00001 || remain_depth < 3 || t >= beta) {
					m = t;
					*move = Moves[i].moves;
				}
				else {
					m = -NegaScout_max_alpha_bet_purning_Original(new_chessboard, transpositionTable ,&new_move, color ^ 1, depth + 1, -beta, -t, remain_depth - 1, Moves[i]); // re-search
					*move = Moves[i].moves;
				}
			}
			
			if (m >= beta) {
				if (HASH_ACTIVE) {
					
					currentHash.value = chessboard.hashValue;
					currentHash.depth = depth;
					currentHash.type = LOWER_BOUND;
					currentHash.score = m;
					currentHash.historyOffset = 0.005 * chessboard.HistoryCount + 0.005;
					currentHash.move = Moves[i].moves;
					
				}
				
				
				this->purn_node_count += move_count - i;
				refutationTable[chessboard.Board[srcPiecePos] / 7][srcPiecePos][dstPiecePos] += 1 << remain_depth;
				assert(*move >= 0);
				return m;
			}
			n = max(alpha, m) + 1;

		}
		
		int srcPiecePos = *move / 100;
		int dstPiecePos = *move % 100;
		assert(*move >= 0);
		refutationTable[chessboard.Board[srcPiecePos] / 7][srcPiecePos][dstPiecePos] += 1 << remain_depth;
		if (HASH_ACTIVE) {
			currentHash.type = (m > alpha) ? EXACT_VALUE : UPPER_BOUND;
			currentHash.depth = depth;
			currentHash.score = m ;
			currentHash.move = *move;
			currentHash.value = chessboard.hashValue;
			currentHash.historyOffset = 0.005 * chessboard.HistoryCount + 0.005;
		}
		
		return m;
	}
}
double MyAI::Nega_max_alpha_bet_purning(ChessBoard chessboard, int* move, int color, const int depth, double alpha, double beta, const int remain_depth) {
	vector<Move> Moves;
	int Chess[2048];
	Move s[2];
	int move_count = 0, flip_count = 0, remain_count = 0, remain_total = 0;
	// move
	move_count = Expand(chessboard.Board, color, Moves);
	MoveOrderSort(chessboard.Board, s, move_count);

	// flip
	for (int j = 0; j < 14; j++) { // find remain chess
		if (chessboard.CoverChess[j] > 0) {
			Chess[remain_count] = j;
			remain_count++;
			remain_total += chessboard.CoverChess[j];
		}
	}
	for (int i = 0; i < 32; i++) { // find cover
		if (chessboard.Board[i] == CHESS_COVER) {
			Moves[move_count + flip_count].moves = i * 100 + i;
			flip_count++;
		}
	}

	// random_shuffle(Moves.begin(), Moves.end());
	if (isTimeUp() || // time is up
		remain_depth == 0 ||   // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count + flip_count == 0 // terminal node (no move type)
		) {

		this->node++;

		// odd: *-1, even: *1
		return Evaluate(&chessboard, move_count + flip_count, color) * (depth & 1 ? -1 : 1);
	}
	else {
		double m = alpha;
		int moves;
		int new_move;
		// search deeper
		for (int i = 0; i < move_count; i++) { // move
			ChessBoard new_chessboard = chessboard;
			// new FriendChessList
			MakeMove(&new_chessboard, Moves[i].moves, 0); // 0: dummy
			double t = -Nega_max_alpha_bet_purning(new_chessboard, &new_move, color ^ 1, depth + 1, -beta, -m, remain_depth - 1);
			if (t > m) {
				m = t;
				*move = Moves[i].moves;
			}

			if (m >= beta) {
				this->purn_node_count += move_count - i;
				return beta;
			}

		}

		//for (int i = move_count; i < move_count + flip_count; i++) { // flip
		//	double total = 0;
		//	for (int k = 0; k < remain_count; k++) {
		//		ChessBoard new_chessboard = chessboard;

		//		MakeMove(&new_chessboard, Moves[i], Chess[k]);
		//		double t = -Nega_max_alpha_bet_purning(new_chessboard, &new_move, color ^ 1, depth + 1, -beta, -m, remain_depth - 1);
		//		total += chessboard.CoverChess[Chess[k]] * t;
		//	}

		//	double E_score = (total / remain_total); // calculate the expect value of flip
		//	if (E_score > m) {
		//		m = E_score;
		//		*move = Moves[i];
		//	}
		//	else if (E_score == m) {
		//		bool r = rand() % 2;
		//		if (r) *move = Moves[i];
		//	}
		//}
		return m;
	}
}
void MyAI::generateMove(char move[6])
{
	/* generateMove Call by reference: change src,dst */
	int StartPoint = 0;
	int EndPoint = 0;
	int stableMove = 0;
	int IDAS_move = 0;
	int begin_depth = 3;
	int limit_depth =  3;
	int myColorTime = (this->Color == 0) ? this->Red_Time : this->Black_Time;
	if (myColorTime < INTER_TIME) {
		begin_depth = 5;
		limit_depth = 5;
		
	}
	else if (main_chessboard.HistoryCount > 20) {
		begin_depth = 4;
		limit_depth = 20;
	}
	
	double t = -DBL_MAX;
	HashEntry* transpositionTable = new HashEntry[1<<20];

	// calculate flip position
	double darkPieceValue = 0;
	int remain_total = 0;
	main_chessboard.initFlipCount = 0;
	for (int j = 0; j < 14; j++) { // find remain chess
		if (main_chessboard.CoverChess[j] > 0) {
			darkPieceValue += (this->Color == j / 7) ? main_chessboard.CoverChess[j] * values[j] : -main_chessboard.CoverChess[j] * values[j];
			remain_total += main_chessboard.CoverChess[j];
		}
	}
	darkPieceValue = (remain_total == 0) ? 0 : darkPieceValue / remain_total;
	for (int i = 0; i < 32; i++) {
		if (main_chessboard.Board[i] >= 0) {
			if (main_chessboard.Board[i] / 7 == this->Color) {
				// friend
				bool needFlip = (values[main_chessboard.Board[i]] + darkPieceValue) > 0;
				if (needFlip) {
					int Moves[4] = { i - 4,i + 1,i + 4,i - 1 };
					for (int k = 0; k < 4; k++)
					{
						if (Moves[k] < 32 && Moves[k] >= 0 && main_chessboard.Board[Moves[k]] == CHESS_COVER) {
							main_chessboard.initFlipPOS[main_chessboard.initFlipCount++] = Moves[k] * 100 + Moves[k];
						}
						
					}
				}

			}
			else {
				if (main_chessboard.Board[i] % 7 == 5)continue;
				//enemy
				bool needFlip = (darkPieceValue) > values[main_chessboard.Board[i]] ;
				if (needFlip) {
					int Moves[4] = { i - 4,i + 1,i + 4,i - 1 };
					for (int k = 0; k < 4; k++)
					{
						if (Moves[k] < 32 && Moves[k] >= 0 && main_chessboard.Board[Moves[k]] == CHESS_COVER) {
							main_chessboard.initFlipPOS[main_chessboard.initFlipCount++] = Moves[k] * 100 + Moves[k];
						}
					}
				}
			}	
		}
	}
	fprintf(stderr, "init move: ");
	for (int i = 0; i < main_chessboard.initFlipCount; i++) {
		fprintf(stderr, " %d ", main_chessboard.initFlipPOS[i]);
		
	}
	fflush(stderr);
	//init refutation table
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 32; j++) {
			for (int k = 0; k < 32; k++) {
				refutationTable[i][j][k] = 0;
			}
		}
	}
	
	Move tempMove;
	begin = clock();
	cout << "[HASH] init_hash: " << this->main_chessboard.hashValue << "\n";
	double best = NegaScout_max_alpha_bet_purning_Original (this->main_chessboard, transpositionTable, &IDAS_move, this->Color, 0, -DBL_MAX, DBL_MAX, IDAS_DEPTH, tempMove);
	lastTime = begin;
	this->oneRound = false;
	// iterative-deeping, start from 3, time limit = <TIME_LIMIT> sec
	for (int depth = begin_depth;  depth <= limit_depth; depth+=2) {
		
		
		if (TIME_LIMIT - (double)(clock() - begin) / CLOCKS_PER_SEC  < 10 * (double)(clock() - lastTime) / CLOCKS_PER_SEC) {
			break;
		}
		lastTime = clock();
		//reduce refutation table
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 32; j++) {
				for (int k = 0; k < 32; k++) {
					assert(refutationTable[i][j][k] >= 0);
					refutationTable[i][j][k] >> (depth > 6)?1:0;
				}
			}
		}
		HashEntry* transpositionTable = new HashEntry[1 << 20];
		HashEntry* transpositionTable2 = new HashEntry[1 << 20];
		this->hashHit = 0;
		this->hashHit2 = 0;
		this->node = 0;
		int best_move;
		double alpha_init = -DBL_MAX;
		double beta_init = DBL_MAX;
		this->purn_node_count = 0;
		
		// run Nega-max
		// t = Nega_max(this->main_chessboard, &best_move, this->Color, 0, depth);
		if (HISTORY_HERUSTIC_ACTIVE) {
			t = NegaScout_max_alpha_bet_purning(this->main_chessboard, &best_move, this->Color, 0, best - IDAS_THRESHOLD, best + IDAS_THRESHOLD, depth);
		}
		else {
			t = NegaScout_max_alpha_bet_purning_Original(this->main_chessboard, transpositionTable ,&best_move, this->Color, 0, best - IDAS_THRESHOLD, best + IDAS_THRESHOLD, depth, tempMove);
		}
		
		fprintf(stderr, "IDAS nodes: %d\n", this->node);
		fflush(stderr);
		if (t <= best - IDAS_THRESHOLD) {
			this->node = 0;
			if (HISTORY_HERUSTIC_ACTIVE) {
				t = NegaScout_max_alpha_bet_purning(this->main_chessboard, &best_move, this->Color, 0, alpha_init, best - IDAS_THRESHOLD, depth);
			}
			else {
				
				t = NegaScout_max_alpha_bet_purning_Original(this->main_chessboard, transpositionTable2, &best_move, this->Color, 0, alpha_init, best - IDAS_THRESHOLD, depth, tempMove);
			}
			fprintf(stderr, "IDAS fail, nodes: %d\n", this->node);
			fflush(stderr);
		}
		else if (t >= best + IDAS_THRESHOLD) {
			this->node = 0;
			if (HISTORY_HERUSTIC_ACTIVE) {
				t = NegaScout_max_alpha_bet_purning(this->main_chessboard, &best_move, this->Color, 0, best + IDAS_THRESHOLD, beta_init, depth);
			}
			else {
				
				t = NegaScout_max_alpha_bet_purning_Original(this->main_chessboard, transpositionTable2 ,&best_move, this->Color, 0, best + IDAS_THRESHOLD, beta_init, depth, tempMove);
			}
			fprintf(stderr, "IDAS fail, nodes: %d\n", this->node);
			fflush(stderr);
		}
		best = t;
		t -= OFFSET; // rescale
		delete [] transpositionTable;
		delete[] transpositionTable2;
		// replace the move and score
		StartPoint = best_move / 100;
		EndPoint = best_move % 100;
		if (!this->timeIsUp) {
			stableMove = best_move;
		}
		sprintf(move, "%c%c-%c%c", 'a' + (StartPoint % 4), '1' + (7 - StartPoint / 4), 'a' + (EndPoint % 4), '1' + (7 - EndPoint / 4));
		fprintf(stderr, "[%c] Depth: %2d, Node: %10d, Score: %+1.5lf, Move: %s, Purn_node_counts: %10d, Hash hit: %10d, Hash hit2: %10d\n", (this->timeIsUp ? 'U' : 'D'),
			depth, node, t, move, this->purn_node_count,this->hashHit, this->hashHit2);
		fflush(stderr);
		this->oneRound = true;
	}
	// replace the move and score
	StartPoint = stableMove / 100;
	EndPoint = stableMove % 100;
	sprintf(move, "%c%c-%c%c", 'a' + (StartPoint % 4), '1' + (7 - StartPoint / 4), 'a' + (EndPoint % 4), '1' + (7 - EndPoint / 4));
	char chess_Start[4] = "";
	char chess_End[4] = "";
	Pirnf_Chess(main_chessboard.Board[StartPoint], chess_Start);
	Pirnf_Chess(main_chessboard.Board[EndPoint], chess_End);
	printf("My result: \n--------------------------\n");
	printf("Nega max: %lf (node: %d)\n", t, this->node);
	printf("(%d) -> (%d)\n", StartPoint, EndPoint);
	printf("<%s> -> <%s>\n", chess_Start, chess_End);
	printf("move:%s\n", move);
	printf("--------------------------\n");
	this->Pirnf_Chessboard();
	delete[] transpositionTable;
}

void MyAI::MakeMove(ChessBoard* chessboard, const int move, const int chess) {
	int src = move / 100, dst = move % 100;
	if (src == dst) { // flip
		chessboard->Board[src] = chess;
		chessboard->CoverChess[chess]--;
		chessboard->NoEatFlip = 0;
		chessboard->hashValue ^= (hashTable[14][src] ^ hashTable[chess][src]); // coverd XOR chess 
		chessboard->hashValue ^= hashColor[0] ^ hashColor[1]; // red XOR black

	}
	else { // move
		chessboard->hashValue ^= hashColor[chessboard->Board[dst] / 7];
		if (chessboard->Board[dst] != CHESS_EMPTY) {
			chessboard->hashValue ^= (hashTable[15][src] ^ hashTable[chessboard->Board[src]][src] ^ hashTable[chessboard->Board[src]][dst] ^ hashTable[chessboard->Board[dst]][dst]);
			if (chessboard->Board[dst] / 7 == 0) { // red
				(chessboard->Red_Chess_Num)--;
				

			}
			else { // black
				(chessboard->Black_Chess_Num)--;
				
			}
			chessboard->remainNum[chessboard->Board[dst]] --;
			chessboard->NoEatFlip = 0;
		}
		else {
			chessboard->hashValue ^= (hashTable[15][src] ^ hashTable[chessboard->Board[src]][src] ^ hashTable[chessboard->Board[src]][dst] ^ hashTable[15][dst]);
			chessboard->NoEatFlip += 1;
		}
		chessboard->Board[dst] = chessboard->Board[src];
		chessboard->Board[src] = CHESS_EMPTY;

	}
	chessboard->History[chessboard->HistoryCount++] = move;
}

void MyAI::MakeMove(ChessBoard* chessboard, const char move[6])
{
	int src, dst, m;
	src = ('8' - move[1]) * 4 + (move[0] - 'a');
	if (move[2] == '(') { // flip
		m = src * 100 + src;
		printf("# call flip(): flip(%d,%d) = %d\n", src, src, GetFin(move[3]));
	}
	else { // move
		dst = ('8' - move[4]) * 4 + (move[3] - 'a');
		m = src * 100 + dst;
		printf("# call move(): move : %d-%d \n", src, dst);
	}
	MakeMove(chessboard, m, ConvertChessNo(GetFin(move[3])));
	Pirnf_Chessboard();
}
bool MyAI::checkQuiescentBoard(ChessBoard* chessboard, const int color, Move& material_exchanging) {
	int gain = 0;
	if (material_exchanging.eat == 1) {
		// eat
		// SSE
		int gain = 0;
		int dst = material_exchanging.moves % 100;
		int boardTemp[32] = { 0 };
		static const double staticValue[14] = {
			  1,180,  6, 18, 90,270,1200,
			  1,180,  6, 18, 90,270,1200
		};
		for (int i = 0; i < 32; i++) {
			boardTemp[i] = chessboard->Board[i];
		}
		bool sse = true;
		int redCounter = 0, blackCounter = 0;
		int colorTemp = color;
		int nextTurn = color ^ 1;
		vector<FriendChessList> red;
		vector<FriendChessList> black;
		int adjacentChess[4] = { dst - 4, dst - 1 ,dst + 1, dst + 4 };
		// TODO: POW
		for (int i = 0; i < 4; i++) {
			if (adjacentChess[i] >= 0 && adjacentChess[i] < 32 && chessboard->Board[adjacentChess[i]] != 1 && chessboard->Board[adjacentChess[i]] != 8) {
				if (chessboard->Board[adjacentChess[i]] / 7 == 0 && chessboard->Board[adjacentChess[i]] >= 0) {
					FriendChessList temp;
					temp.value = chessboard->Board[adjacentChess[i]];

					temp.i = adjacentChess[i];
					red.push_back(temp);
				}
				else if (chessboard->Board[adjacentChess[i]] / 7 == 1 && chessboard->Board[adjacentChess[i]] >= 0) {
					FriendChessList temp;
					temp.value = chessboard->Board[adjacentChess[i]];
					temp.i = adjacentChess[i];
					black.push_back(temp);
				}

			}
		}
		
		sort(red.begin(), red.end(), [](FriendChessList a, FriendChessList b) {return a.value < b.value; });
		sort(black.begin(), black.end(), [](FriendChessList a, FriendChessList b) {return a.value < b.value; });

		while (red.size() > 0 && black.size() > 0) {
			if (nextTurn == 0) {
				gain += staticValue[boardTemp[red.back().value]] * (this->Color == 0) ? 1 : -1;
				boardTemp[red.back().i] = CHESS_EMPTY;
				boardTemp[dst] = boardTemp[red.back().i];
				red.pop_back();
			}
			else {
				gain += staticValue[boardTemp[black.back().value]] * (this->Color == 0) ? 1 : -1;;
				boardTemp[red.back().i] = CHESS_EMPTY;
				boardTemp[dst] = boardTemp[black.back().i];
				red.pop_back();
			}
			nextTurn ^= 1;
		}


	}
	return gain > 0;
}
int MyAI::Expand(const int* board, const int color, vector<Move>& Result)
{
	int ResultCount = 0;
	for (int i = 0; i < 32; i++)
	{
		if (board[i] >= 0 && board[i] / 7 == color)
		{
			//Gun
			if (board[i] % 7 == 1)
			{
				int row = i / 4;
				int col = i % 4;
				for (int rowCount = row * 4; rowCount < (row + 1) * 4; rowCount++)
				{
					if (Referee(board, i, rowCount, color))
					{
						Move temp;
						temp.moves = i * 100 + rowCount;
						temp.priority = board[i] / 7;
						Result.push_back(temp);
					}
				}
				for (int colCount = col; colCount < 32; colCount += 4)
				{

					if (Referee(board, i, colCount, color))
					{
						Move temp;
						temp.moves = i * 100 + colCount;
						temp.priority = board[i] / 7;
						Result.push_back(temp);
					}
				}
			}
			else
			{
				int Moves[4] = { i - 4,i + 1,i + 4,i - 1 };
				for (int k = 0; k < 4; k++)
				{

					if (Moves[k] >= 0 && Moves[k] < 32 && Referee(board, i, Moves[k], color))
					{
						Move temp;
						temp.moves = i * 100 + Moves[k];
						temp.priority = board[i] / 7;
						Result.push_back(temp);
					}
				}
			}

		};
	}
	ResultCount = Result.size();
	return ResultCount;
}
int MyAI::Expand(const int* board, const int color, int* Result)
{
	int ResultCount = 0;
	for (int i = 0; i < 32; i++)
	{
		if (board[i] >= 0 && board[i] / 7 == color)
		{
			//Gun
			if (board[i] % 7 == 1)
			{
				int row = i / 4;
				int col = i % 4;
				for (int rowCount = row * 4; rowCount < (row + 1) * 4; rowCount++)
				{
					if (Referee(board, i, rowCount, color))
					{
						Result[ResultCount] = i * 100 + rowCount;
						ResultCount++;
					}
				}
				for (int colCount = col; colCount < 32; colCount += 4)
				{

					if (Referee(board, i, colCount, color))
					{
						Result[ResultCount] = i * 100 + colCount;
						ResultCount++;
					}
				}
			}
			else
			{
				int Move[4] = { i - 4,i + 1,i + 4,i - 1 };
				for (int k = 0; k < 4; k++)
				{

					if (Move[k] >= 0 && Move[k] < 32 && Referee(board, i, Move[k], color))
					{
						Result[ResultCount] = i * 100 + Move[k];
						ResultCount++;
					}
				}
			}

		};
	}
	return ResultCount;
}
int MyAI::Expand(ChessBoard* chessboard, const int* board, const int color, Move* Result, Move* opResult, int& opCount)
{
	int ResultCount = 0;
	for (int i = 0; i < 32; i++)
	{
		
		
		if (board[i] >= 0 && board[i] / 7 == color)
		{
			
			//Gun
			if (board[i] % 7 == 1)
			{
				int row = i / 4;
				int col = i % 4;
				for (int rowCount = row * 4; rowCount < (row + 1) * 4; rowCount++)
				{
					if (Referee(board, i, rowCount, color))
					{
						Result[ResultCount].moves = i * 100 + rowCount;
						Result[ResultCount].priority = board[i] % 7;
						Result[ResultCount++].eat = (board[rowCount] >= 0) ? 1 : 0;
					}
				}
				for (int colCount = col; colCount < 32; colCount += 4)
				{

					if (Referee(board, i, colCount, color))
					{
						Result[ResultCount].moves = i * 100 + colCount;
						Result[ResultCount].priority = board[i] % 7;
						Result[ResultCount++].eat = (board[colCount] >= 0) ? 1 : 0;

					}
				}
			}
			else
			{
				int Moves[4] = { i - 4,i + 1,i + 4,i - 1 };
				for (int k = 0; k < 4; k++)
				{

					if (Moves[k] >= 0 && Moves[k] < 32 && Referee(board, i, Moves[k], color))
					{
						Result[ResultCount].moves = i * 100 + Moves[k];
						Result[ResultCount].priority = board[i] % 7;
						Result[ResultCount++].eat = (board[Moves[k]] >= 0) ? 1 : 0;
					}
				}
			}

		}
		else if (board[i] >= 0 && board[i] / 7 != color) {
			//Gun
			if (board[i] % 7 == 1)
			{
				int row = i / 4;
				int col = i % 4;
				for (int rowCount = row * 4; rowCount < (row + 1) * 4; rowCount++)
				{
					if (Referee(board, i, rowCount, color ^ 1))
					{
						opResult[opCount].moves = i * 100 + rowCount;
						opResult[opCount].priority = board[i] % 7;
						opResult[opCount++].eat = (board[rowCount] >= 0) ? 1 : 0;
						
					}
				}
				for (int colCount = col; colCount < 32; colCount += 4)
				{

					if (Referee(board, i, colCount, color ^ 1))
					{
						opResult[opCount].moves = i * 100 + colCount;
						opResult[opCount].priority = board[i] % 7;
						opResult[opCount++].eat = (board[colCount] >= 0) ? 1 : 0;
					}
				}
			}
			else
			{
				int Moves[4] = { i - 4,i + 1,i + 4,i - 1 };
				for (int k = 0; k < 4; k++)
				{

					if (Moves[k] >= 0 && Moves[k] < 32 && Referee(board, i, Moves[k], color ^ 1))
					{
						opResult[opCount].moves = i * 100 + Moves[k];
						opResult[opCount].priority = board[i] % 7;
						opResult[opCount++].eat = (board[Moves[k]] >= 0) ? 1 : 0;
					}
				}
			}
		}
		
	}
	return ResultCount;
}
// Referee
bool MyAI::Referee(const int* chess, const int from_location_no, const int to_location_no, const int UserId)
{
	// int MessageNo = 0;
	bool IsCurrent = true;
	int from_chess_no = chess[from_location_no];
	int to_chess_no = chess[to_location_no];
	int from_row = from_location_no / 4;
	int to_row = to_location_no / 4;
	int from_col = from_location_no % 4;
	int to_col = to_location_no % 4;

	if (from_chess_no < 0 || (to_chess_no < 0 && to_chess_no != CHESS_EMPTY))
	{
		// MessageNo = 1;
		//strcat(Message,"**no chess can move**");
		//strcat(Message,"**can't move darkchess**");
		IsCurrent = false;
	}
	else if (from_chess_no >= 0 && from_chess_no / 7 != UserId)
	{
		// MessageNo = 2;
		//strcat(Message,"**not my chess**");
		IsCurrent = false;
	}
	else if ((from_chess_no / 7 == to_chess_no / 7) && to_chess_no >= 0)
	{
		// MessageNo = 3;
		//strcat(Message,"**can't eat my self**");
		IsCurrent = false;
	}
	//check attack
	else if (to_chess_no == CHESS_EMPTY && abs(from_row - to_row) + abs(from_col - to_col) == 1)//legal move
	{
		IsCurrent = true;
	}
	else if (from_chess_no % 7 == 1) //judge gun
	{
		int row_gap = from_row - to_row;
		int col_gap = from_col - to_col;
		int between_Count = 0;
		//slant
		if (from_row - to_row == 0 || from_col - to_col == 0)
		{
			//row
			if (row_gap == 0)
			{
				for (int i = 1; i < abs(col_gap); i++)
				{
					int between_chess;
					if (col_gap > 0)
						between_chess = chess[from_location_no - i];
					else
						between_chess = chess[from_location_no + i];
					if (between_chess != CHESS_EMPTY)
						between_Count++;
				}
			}
			//column
			else
			{
				for (int i = 1; i < abs(row_gap); i++)
				{
					int between_chess;
					if (row_gap > 0)
						between_chess = chess[from_location_no - 4 * i];
					else
						between_chess = chess[from_location_no + 4 * i];
					if (between_chess != CHESS_EMPTY)
						between_Count++;
				}

			}

			if (between_Count != 1)
			{
				// MessageNo = 4;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
			else if (to_chess_no == CHESS_EMPTY)
			{
				// MessageNo = 5;
				//strcat(Message,"**gun can't eat opp without between one piece**");
				IsCurrent = false;
			}
		}
		//slide
		else
		{
			// MessageNo = 6;
			//strcat(Message,"**cant slide**");
			IsCurrent = false;
		}
	}
	else // non gun
	{
		//judge pawn or king

		//distance
		if (abs(from_row - to_row) + abs(from_col - to_col) > 1)
		{
			// MessageNo = 7;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
		//judge pawn
		else if (from_chess_no % 7 == 0)
		{
			if (to_chess_no % 7 != 0 && to_chess_no % 7 != 6)
			{
				// MessageNo = 8;
				//strcat(Message,"**pawn only eat pawn and king**");
				IsCurrent = false;
			}
		}
		//judge king
		else if (from_chess_no % 7 == 6 && to_chess_no % 7 == 0)
		{
			// MessageNo = 9;
			//strcat(Message,"**king can't eat pawn**");
			IsCurrent = false;
		}
		else if (from_chess_no % 7 < to_chess_no % 7)
		{
			// MessageNo = 10;
			//strcat(Message,"**cant eat**");
			IsCurrent = false;
		}
	}
	return IsCurrent;
}

// always use my point of view, so use this->Color
double MyAI::Evaluate(const ChessBoard* chessboard,
	const int legal_move_count, const int color) {
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero

	double score = OFFSET;
	double myMobilityScore = 0;
	double opMobilityScore = 0;
	bool finish = false;

	if (legal_move_count == 0) { // Win, Lose
		if (color == this->Color) { // Lose
			score += LOSE - WIN;
		}
		else { // Win
			score += WIN - LOSE;
		}
		finish = true;
	}
	else if (isDraw(chessboard)) { // Draw
	   // score += DRAW - DRAW;
		score += LOSE - WIN;
		score -= 0.3;
	}
	else { // no conclusion
	   // static material values
	   // cover and empty are both zero
		

		double piece_value = 0;
		int myColorNum = (this->Color == chessboard->Red_Chess_Num) ? (14 - chessboard->Red_Chess_Num) : (14 - chessboard->Black_Chess_Num);
		for (int i = 0; i < 32; i++) {
			if (chessboard->Board[i] != CHESS_EMPTY) {
				if (chessboard->Board[i] != CHESS_COVER) {
					if (chessboard->Board[i] / 7 == this->Color) {
						piece_value += values[chessboard->Board[i]];
					}
					else {
						piece_value -= values[chessboard->Board[i]];
					}
				}
				else {
					piece_value += 166 * myColorNum - 166 * chessboard->Black_Chess_Num;
				}
			}
		}
		/*for (int i = 0; i < 14; ++i) {

			if (this->Color == (i / 7))myMobilityScore += values[i] * 0.01 * pieceMoveCount[i];
			else opMobilityScore += values[i] * 0.01 * pieceMoveCount[i];

		}*/
		// linear map to (-<WIN>, <WIN>)
		// score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
		// <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * (<WIN> - 0.01)
		piece_value = piece_value / 1943 * (WIN - 0.01);
		score += piece_value;
		finish = false;
	}

	// Bonus (Only Win / Draw)
	if (finish) {
		if ((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)) {
			if (!(legal_move_count == 0 && color == this->Color)) { // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE *
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if (bonus > BONUS) { bonus = BONUS; } // limit
				score += bonus;
			}
		}
		else if ((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)) {
			if (!(legal_move_count == 0 && color != this->Color)) { // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE *
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if (bonus > BONUS) { bonus = BONUS; } // limit
				score -= bonus;
			}
		}
	}

	return score;
}

double MyAI::Evaluate(const ChessBoard* chessboard,
	const int legal_move_count, const int color, Move* Result, Move* opResult, int& moveCount, int& opMoveCount) {
	// score = My Score - Opponent's Score
	// offset = <WIN + BONUS> to let score always not less than zero
	int singleMoveCount[14] = { 0 };
	double dominateScore = 0;
	double score = OFFSET;
	double myMobilityScore = 0;
	double opMobilityScore = 0;
	bool finish = false;
	
	for (int i = 0; i < moveCount; i++) {
		int srcPiece = Result[i].moves / 100;
		singleMoveCount[chessboard->Board[srcPiece]] ++;
	}
	for (int i = 0; i < opMoveCount; i++) {
		int srcPiece = opResult[i].moves / 100;
		singleMoveCount[chessboard->Board[srcPiece]] ++;
	}
	if (legal_move_count == 0) { // Win, Lose
		if (color == this->Color) { // Lose
			score += LOSE - WIN;
		}
		else { // Win
			score += WIN - LOSE;
			score += WIN - LOSE;
		}
		finish = true;
	}
	else if (isDraw(chessboard)) { // Draw
	   // score += DRAW - DRAW;
		score += LOSE - WIN;
		score -= 13;
	}
	else { // no conclusion
	   // static material values
	   // cover and empty are both zero
		

		double piece_value = 0;
		int myColorNum = (this->Color == chessboard->Red_Chess_Num) ? (14 - chessboard->Red_Chess_Num) : (14 - chessboard->Black_Chess_Num);
		for (int i = 0; i < 32; i++) {
			if (chessboard->Board[i] != CHESS_EMPTY) {
				if (chessboard->Board[i] != CHESS_COVER) {
					if (chessboard->Board[i] / 7 == this->Color) {
						piece_value += values[chessboard->Board[i]];
					}
					else {
						piece_value -= values[chessboard->Board[i]];
					}
				}
				else {
					piece_value += chessboard->darkPieceValue;
				}
			}
		}
		if (chessboard->remainNum[0] > 0 && chessboard->remainNum[13] > 0 && 1 == this->Color) {
			piece_value -= 100 * chessboard->remainNum[0];
		}
		if (chessboard->remainNum[7] > 0 && chessboard->remainNum[6] > 0 && 0 == this->Color) {
			piece_value -= 100 * chessboard->remainNum[7];
		}

		/*for (int i = 0; i < 14; ++i) {

			if (this->Color == (i / 7))myMobilityScore += values[i] * 0.01 * pieceMoveCount[i];
			else opMobilityScore += values[i] * 0.01 * pieceMoveCount[i];

		}*/
		// linear map to (-<WIN>, <WIN>)
		// score max value = 1*5 + 180*2 + 6*2 + 18*2 + 90*2 + 270*2 + 810*1 = 1943
		// <ORIGINAL_SCORE> / <ORIGINAL_SCORE_MAX_VALUE> * (<WIN> - 0.01)
		
		piece_value += 0.75 * myMobilityScore -  opMobilityScore;
		piece_value = piece_value / 1943 * (WIN - 0.01) ;
		score += piece_value ;
		finish = false;
	}
	
	// Bonus (Only Win / Draw)
	if (finish) {
		if ((this->Color == RED && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num)) {
			if (!(legal_move_count == 0 && color == this->Color)) { // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE *
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if (bonus > BONUS) { bonus = BONUS; } // limit
				score += bonus;
			}
		}
		else if ((this->Color == RED && chessboard->Red_Chess_Num < chessboard->Black_Chess_Num) ||
			(this->Color == BLACK && chessboard->Red_Chess_Num > chessboard->Black_Chess_Num)) {
			if (!(legal_move_count == 0 && color != this->Color)) { // except Lose
				double bonus = BONUS / BONUS_MAX_PIECE *
					abs(chessboard->Red_Chess_Num - chessboard->Black_Chess_Num);
				if (bonus > BONUS) { bonus = BONUS; } // limit
				score -= bonus;
			}
		}
	}

	return score - 0.005 * chessboard->HistoryCount ;
}
double MyAI::Nega_max(const ChessBoard chessboard, int* move, const int color, const int depth, const int remain_depth) {
	vector<Move> s;
	int Moves[2048], Chess[2048];
	int move_count = 0, flip_count = 0, remain_count = 0, remain_total = 0;

	// move
	move_count = Expand(chessboard.Board, color, s);
	// flip
	for (int j = 0; j < 14; j++) { // find remain chess
		if (chessboard.CoverChess[j] > 0) {
			Chess[remain_count] = j;
			remain_count++;
			remain_total += chessboard.CoverChess[j];
		}
	}
	for (int i = 0; i < 32; i++) { // find cover
		if (chessboard.Board[i] == CHESS_COVER) {
			Moves[move_count + flip_count] = i * 100 + i;
			flip_count++;
		}
	}

	if (remain_depth <= 0 || // reach limit of depth
		chessboard.Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard.Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count + flip_count == 0 // terminal node (no move type)
		) {
		this->node++;
		// odd: *-1, even: *1
		return Evaluate(&chessboard, move_count + flip_count, color) * (depth & 1 ? -1 : 1);
	}
	else {
		double m = -DBL_MAX;
		int new_move;
		// search deeper
		for (int i = 0; i < move_count; i++) { // move
			ChessBoard new_chessboard = chessboard;

			MakeMove(&new_chessboard, Moves[i], 0); // 0: dummy
			double t = -Nega_max(new_chessboard, &new_move, color ^ 1, depth + 1, remain_depth - 1);
			if (t > m) {
				m = t;
				*move = Moves[i];
			}
			else if (t == m) {
				bool r = rand() % 2;
				if (r) *move = Moves[i];
			}
		}
		for (int i = move_count; i < move_count + flip_count; i++) { // flip
			double total = 0;
			for (int k = 0; k < remain_count; k++) {
				ChessBoard new_chessboard = chessboard;

				MakeMove(&new_chessboard, Moves[i], Chess[k]);
				double t = -Nega_max(new_chessboard, &new_move, color ^ 1, depth + 1, remain_depth - 1);
				total += chessboard.CoverChess[Chess[k]] * t;
			}

			double E_score = (total / remain_total); // calculate the expect value of flip
			if (E_score > m) {
				m = E_score;
				*move = Moves[i];
			}
			else if (E_score == m) {
				bool r = rand() % 2;
				if (r) *move = Moves[i];
			}
		}
		return m;
	}
}

bool MyAI::isDraw(const ChessBoard* chessboard) {
	// No Eat Flip
	if (chessboard->NoEatFlip >= NOEATFLIP_LIMIT) {
		return true;
	}

	// Position Repetition
	int last_idx = chessboard->HistoryCount - 1;
	// -2: my previous ply
	int idx = last_idx - 2;
	// All ply must be move type
	int smallest_repetition_idx = last_idx - (chessboard->NoEatFlip / POSITION_REPETITION_LIMIT);
	// check loop
	while (idx >= 0 && idx >= smallest_repetition_idx) {
		if (chessboard->History[idx] == chessboard->History[last_idx]) {
			// how much ply compose one repetition
			int repetition_size = last_idx - idx;
			bool isEqual = true;
			for (int i = 1; i < POSITION_REPETITION_LIMIT && isEqual; ++i) {
				for (int j = 0; j < repetition_size; ++j) {
					int src_idx = last_idx - j;
					int checked_idx = last_idx - i * repetition_size - j;
					if (chessboard->History[src_idx] != chessboard->History[checked_idx]) {
						isEqual = false;
						break;
					}
				}
			}
			if (isEqual) {
				return true;
			}
		}
		idx -= 2;
	}

	return false;
}

bool MyAI::isFinish(const ChessBoard* chessboard, int move_count) {
	return (
		chessboard->Red_Chess_Num == 0 || // terminal node (no chess type)
		chessboard->Black_Chess_Num == 0 || // terminal node (no chess type)
		move_count == 0 || // terminal node (no move type)
		isDraw(chessboard) // draw
		);
}

//Display chess board
void MyAI::Pirnf_Chessboard()
{
	char Mes[1024] = "";
	char temp[1024];
	char myColor[10] = "";
	if (Color == -99)
		strcpy(myColor, "Unknown");
	else if (this->Color == RED)
		strcpy(myColor, "Red");
	else
		strcpy(myColor, "Black");
	sprintf(temp, "------------%s-------------\n", myColor);
	strcat(Mes, temp);
	strcat(Mes, "<8> ");
	for (int i = 0; i < 32; i++) {
		if (i != 0 && i % 4 == 0) {
			sprintf(temp, "\n<%d> ", 8 - (i / 4));
			strcat(Mes, temp);
		}
		char chess_name[4] = "";
		Pirnf_Chess(this->main_chessboard.Board[i], chess_name);
		sprintf(temp, "%5s", chess_name);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n     ");
	for (int i = 0; i < 4; i++) {
		sprintf(temp, " <%c> ", 'a' + i);
		strcat(Mes, temp);
	}
	strcat(Mes, "\n\n");
	printf("%s", Mes);
}


// Print chess
void MyAI::Pirnf_Chess(int chess_no, char* Result) {
	// XX -> Empty
	if (chess_no == CHESS_EMPTY) {
		strcat(Result, " - ");
		return;
	}
	// OO -> DarkChess
	else if (chess_no == CHESS_COVER) {
		strcat(Result, " X ");
		return;
	}

	switch (chess_no) {
	case 0:
		strcat(Result, " P ");
		break;
	case 1:
		strcat(Result, " C ");
		break;
	case 2:
		strcat(Result, " N ");
		break;
	case 3:
		strcat(Result, " R ");
		break;
	case 4:
		strcat(Result, " M ");
		break;
	case 5:
		strcat(Result, " G ");
		break;
	case 6:
		strcat(Result, " K ");
		break;
	case 7:
		strcat(Result, " p ");
		break;
	case 8:
		strcat(Result, " c ");
		break;
	case 9:
		strcat(Result, " n ");
		break;
	case 10:
		strcat(Result, " r ");
		break;
	case 11:
		strcat(Result, " m ");
		break;
	case 12:
		strcat(Result, " g ");
		break;
	case 13:
		strcat(Result, " k ");
		break;
	}
}


