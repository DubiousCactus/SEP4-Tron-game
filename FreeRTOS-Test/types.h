#define MAXTURNS 14

typedef struct Turn {
	int x;
	int y;
} Turn;

typedef enum Direction {
	LEFT, RIGHT, UP, DOWN
} Direction;

typedef struct Player {
	int x;
	int y;
	enum Direction direction;
	Turn turns[MAXTURNS + 1];
	int turnsCount;
} Player;

typedef struct Score {
	int playerOne;
	int playerTwo;
} Score;
