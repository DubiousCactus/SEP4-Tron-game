#define MAXTURNS 14
#define MAX_PAYLOAD_LENGTH 10
#define MAX_TRAILER_LENGTH 10
#define FLAG 0xFF
#define ESCAPE 0xDD
#define COMMA 0x2c
#define DOT 0x2E

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

