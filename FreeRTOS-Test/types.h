typedef struct Turn {
	int x;
	int y;
} Turn;

typedef enum Direction {
	LEFT, RIGHT, UP, DOWN
} Direction;

typedef struct Position {
	int x;
	int y;
	enum Direction direction;
	Turn turns[1];
} Position;

typedef struct Score {
	int playerOne;
	int playerTwo;
} Score;
