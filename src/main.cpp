#include "asteroids/game.cpp"

int main(const int argc, const char** const argv)
{
    Game game;
    
    game.init(1280, 720);
    game.main();
    game.cleanup();

    return 0;
}
