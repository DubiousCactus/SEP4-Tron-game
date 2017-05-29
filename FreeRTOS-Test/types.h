typedef struct Turn {
	int x;
	int y;
} Turn;

enum Direction {
	LEFT, RIGHT, UP, DOWN
};

typedef struct Position {
	int x;
	int y;
	enum Direction direction;
	Turn turns[];
} Position;

typedef struct Score {
	int playerOne;
	int playerTwo;
} Score;
