# 725OS
725OS is my small x86-64 UEFI project. Right now, it boots straight into a graphical Pac-Man game instead of a desktop or command line. I’m using it to build up the pieces of an operating system one step at a time.
The game draws through the UEFI Graphics Output Protocol (GOP), takes keyboard input, and supports a UEFI mouse when one is available. It also saves the player name and high score in UEFI non-volatile storage.
## What’s in the project
- `boot/boot.c` starts the game, shows the home/name page, and handles the timer and restart flow.
- `game/game.c` contains the maze, pellet respawning, scoring, ghost pathfinding, and difficulty changes.
- `drivers/display/framebuffer.c` draws the game and supplied sprites.
- `drivers/keyboard/keyboard.c` handles name entry and keyboard controls.
- `drivers/keyboard/mouse.c` adds mouse support for the on-screen buttons when UEFI provides a pointer.
- `drivers/storage/score_store.c` saves the player name and high score.
- `assets/sprites/` holds the artwork, and `scripts/pack_assets.py` packages the PNGs into the EFI app during the build.
Ghosts get faster as the game progresses, follow paths around walls, and join in at 1,000, 2,500, 4,500, and 7,000 points. Collected pellets return after about 12 seconds, so the game continues until Pac-Man is caught.
## Controls
I can move with the arrow keys or WASD. R or the Restart button starts a new game. Esc or the Home button returns to the home/name page. Each arrow or WASD input moves one tile. Holding a key can trigger repeated movement events from UEFI, so Pac-Man can move continuously while the key is held.
## What I want to add next
I’d like to keep improving 725OS as I learn more about operating systems. Some things I plan to work on are audio drivers and game sound, smoother input and animation, more gameplay features, and better hardware support.
Check out the BUILD IMAGE folder for snapshots of the project.
