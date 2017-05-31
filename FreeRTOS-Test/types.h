#define MAXTURNS 14
#define FLAG 0xFF
#define ESCAPE 0xDD

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

typedef enum ProtocolState {
	IDLE, ESC, HEADER, ERROR, PAYLOAD, TRAILER, FRAME_VALIDATION
} ProtocolState;
