# Snake-Multiplayer-Server
 This repository is an implementation of the Snake game server. This project is carried out for the distributed computing course

The project runs on linux. 

how to run:
    make run

how to debug:
    make debug

TODO
0. Create readme - Done
1. Fix comments
2. Add documentation (for ex. how many bytes are we sending/reading to/from clien)
3. Implement game logic after snake death - implemented
4. Fix initializing of snake (we don't want to spawn new snake near border)
5. Add color to each snake in Snakes array in Game struct and send this color do client so he can distinguish his snake from others
6. Refactor code
