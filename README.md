
# Snake-Multiplayer-Server

This repository is an implementation of the Snake game server. This project is carried out for the distributed computing course.


## Authors

- [@witczakxd](https://www.github.com/witczakxd)
- [@abialek677](https://www.github.com/abialek677)
- [@wolekhenryk](https://www.github.com/wolekhenryk)
- [@mgdkmp13](https://www.github.com/mgdkmp13)


## Tech Stack

<p align="left">
<a href="https://docs.microsoft.com/en-us/cpp/?view=msvc-170" target="_blank" rel="noreferrer"><img src="https://raw.githubusercontent.com/danielcranney/readme-generator/main/public/icons/skills/c-colored.svg" width="36" height="36" alt="C" /></a><a href="https://www.python.org/" target="_blank" rel="noreferrer"><img src="https://raw.githubusercontent.com/danielcranney/readme-generator/main/public/icons/skills/python-colored.svg" width="36" height="36" alt="Python" /></a>
</p>
## Run Locally on Linux

Clone the project

```bash
  git clone https://github.com/witczakxd/Snake-Multiplayer-Server
```

Go to the project directory

```bash
  cd Snake-Multiplayer-Server
```

Run the server

```bash
  make run
```

**or**

*Debug* the server

```bash
  make debug
```


## TODO

- Create readme - ✅
- Fix comments - ✅
- Implement game logic after snake death - ✅
- Add documentation (for ex. how many bytes are we sending/reading to/from client)
- Fix initializing of snake (we don't want to spawn new snake near border)
- Add color to each snake in Snakes array in Game struct and send this color do client so he can distinguish his snake from others
- Refactor code
