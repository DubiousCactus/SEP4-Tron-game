class MovePlayerTest {

    enum Direction {
        LEFT, RIGHT, UP, DOWN
    };

    /* Replicate C's structures */
    class Player {
        public Direction direction;
        public int x;
        public int y;
    }

    Player player;

    /* Function to be tested */
    void move_player(Player player) {
        switch(player.direction) {
            case LEFT:
                if (player.x > 0)
                    player.x--;
                break;
            case RIGHT:
                if (player.x < 14)
                    player.x++;
                break;
            case UP:
                if (player.y > 0)
                    player.y--;
                break;
            case DOWN:
                if (player.y < 10)
                    player.y++;
                break;
        }
    }


    @Test
    void main() {

        //Normal in bounds value
        player.x = 3;
        player.direction = Direction.LEFT;
        move_player(player);
        assert player.x == 2;

        //Out of bounds value
        player.x = 0;
        player.direction = Direction.LEFT;
        move_player(player);
        assert player.x == 0;


        //Normal in bounds value
        player.x = 13;
        player.direction = Direction.RIGHT;
        move_player(player);
        assert player.x == 14;

        //Out of bounds value
        player.x = 14;
        player.direction = Direction.RIGHT;
        move_player(player);
        assert player.x == 14;


        //Normal in bounds value
        player.x = 2;
        player.direction = Direction.UP;
        move_player(player);
        assert player.x == 1;

        //Out of bounds value
        player.x = 0;
        player.direction = Direction.UP;
        move_player(player);
        assert player.x == 0;


        //Normal in bounds value
        player.x = 9;
        player.direction = Direction.DOWN;
        move_player(player);
        assert player.x == 10;

        //Out of bounds value
        player.x = 10;
        player.direction = Direction.DOWN;
        move_player(player);
        assert player.x == 10;
    }

    @BeforeEach
    void setUp() {
        player = new Player();
    }

}
