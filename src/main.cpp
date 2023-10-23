#include "testGame.cpp"

int main(const int argc, const char** const argv)
{
    TestGame game;
    
    game.init(1280, 720);
    game.main();
    game.cleanup();

    return 0;
}
